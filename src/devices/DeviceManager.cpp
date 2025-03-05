#include "DeviceManager.h"
#include "mosquitto/MosquittoDeviceManager.h"
#include "zigbee/ZigbeeDeviceManager.h"
#include "esphome/ESPHomeDeviceManager.h"
#include "ddp/DDPDeviceManager.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>

DeviceManager::DeviceManager(ResourceManagerInterface* resource_manager, QObject* parent)
    : QObject(parent)
    , resource_manager(resource_manager)
    , mosquitto_manager(nullptr)
    , zigbee_manager(nullptr)
    , esphome_manager(nullptr)
    , ddp_manager(nullptr)
    , update_timer(new QTimer(this))
    , config_manager(nullptr)
    , mqtt_handler(nullptr)
    , post_discovery_timer(nullptr)
{
    update_timer->setSingleShot(true);
    connect(update_timer, &QTimer::timeout, this, &DeviceManager::updateDeviceList);

    // Initialize managers with proper error handling
    try {
        // Initialize mosquitto manager
        mosquitto_manager = new MosquittoDeviceManager(this);
        if (mosquitto_manager) {
            connect(mosquitto_manager, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
                    this, SIGNAL(mqttPublishNeeded(QString,QByteArray)));
            connect(mosquitto_manager, SIGNAL(deviceListChanged()),
                    this, SLOT(onProtocolDevicesChanged()));
        }
    } catch (const std::exception& e) {
        qWarning() << "[DeviceManager] Error initializing MosquittoDeviceManager:" << e.what();
        mosquitto_manager = nullptr;
    }

    try {
        // Initialize zigbee manager
        zigbee_manager = new ZigbeeDeviceManager(this);
        if (zigbee_manager) {
            connect(zigbee_manager, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
                    this, SIGNAL(mqttPublishNeeded(QString,QByteArray)));
            connect(zigbee_manager, SIGNAL(deviceListChanged()),
                    this, SLOT(onProtocolDevicesChanged()));
        }
    } catch (const std::exception& e) {
        qWarning() << "[DeviceManager] Error initializing ZigbeeDeviceManager:" << e.what();
        zigbee_manager = nullptr;
    }

    // ESPHome is known to be problematic, so wrap in try/catch
    try {
        // Initialize ESPHome manager but don't connect signals yet - known to be problematic
        esphome_manager = new ESPHomeDeviceManager(this);
        if (esphome_manager) {
            connect(esphome_manager, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
                    this, SIGNAL(mqttPublishNeeded(QString,QByteArray)));
            connect(esphome_manager, SIGNAL(deviceListChanged()),
                    this, SLOT(onProtocolDevicesChanged()));
        }
    } catch (const std::exception& e) {
        qWarning() << "[DeviceManager] Error initializing ESPHomeDeviceManager:" << e.what();
        esphome_manager = nullptr;
    }
}

DeviceManager::~DeviceManager()
{
    delete mosquitto_manager;
    delete zigbee_manager;
    delete esphome_manager;
    delete update_timer;
    // Note: ddp_manager is owned externally, don't delete it here
}

void DeviceManager::setDDPDeviceManager(DDPDeviceManager* manager)
{
    if (ddp_manager == manager) return;
    
    if (ddp_manager) {
        disconnect(ddp_manager, SIGNAL(deviceListChanged()),
                this, SLOT(onProtocolDevicesChanged()));
    }
    
    ddp_manager = manager;
    
    if (ddp_manager) {
        connect(ddp_manager, SIGNAL(deviceListChanged()),
                this, SLOT(onProtocolDevicesChanged()));
    }
}

void DeviceManager::setMQTTHandler(QObject* handler)
{
    mqtt_handler = handler;
    // MQTT handler has been set
}

void DeviceManager::setConfigManager(ConfigManager* manager)
{
    config_manager = manager;
    
    // If we now have a config manager, load the saved device states
    if (config_manager) {
        // Load saved device states from config
        
        // First, establish a connection to save config on shutdown
        connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
            if (config_manager) {
                // Force save all devices currently in enabled list
                for (const auto& pair : devices_added_to_openrgb) {
                    config_manager->setDeviceEnabled(pair.first, pair.second);
                }
                
                // Force a final save
                config_manager->saveConfig(config_manager->config_file);
            }
        });
        
        // Load the enabled device status directly from config
        QJsonObject config_obj;
        QFile file(config_manager->config_file);
        
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (!doc.isNull() && doc.isObject()) {
                config_obj = doc.object();
                
                // Check if we have enabled_devices section
                if (config_obj.contains("enabled_devices") && config_obj["enabled_devices"].isObject()) {
                    QJsonObject enabled_devices = config_obj["enabled_devices"].toObject();
                    
                    for (const QString& key : enabled_devices.keys()) {
                        bool enabled = enabled_devices[key].toBool();
                        
                        if (enabled) {
                            devices_added_to_openrgb[key.toStdString()] = true;
                        }
                    }
                }
            }
            file.close();
        }
        
        // Get all available devices and check their saved state as a backup
        auto all_devices = getAllAvailableDevices();
        
        for (const auto& device_info : all_devices) {
            const std::string& name = device_info.first;
            bool should_be_enabled = config_manager->isDeviceEnabled(name);
            
            if (should_be_enabled) {
                devices_added_to_openrgb[name] = true;
            }
        }
        
        // Don't schedule an immediate update - we need to wait for device discovery first
        // Instead, we'll connect a signal to trigger registration when devices are discovered
        
        // Set up a post-discovery device registration handler
        static bool connection_established = false;
        if (!connection_established) {
            connection_established = true;
            
            // This handler will run after device discovery is complete
            auto postDiscoveryHandler = [this]() {
                // Delay slightly to ensure all protocols have had a chance to report their devices
                QTimer::singleShot(2000, this, [this]() {
                    // Reapply the enabled states for all discovered devices
                    auto all_devices = getAllAvailableDevices();
                    
                    // Rebuild the devices_added_to_openrgb map based on discovered devices and saved config
                    for (const auto& device_info : all_devices) {
                        const std::string& name = device_info.first;
                        bool should_be_enabled = config_manager->isDeviceEnabled(name);
                        devices_added_to_openrgb[name] = should_be_enabled;
                    }
                    
                    // Now update the device list with the refreshed map
                    updateDeviceList();
                });
            };
            
            // Set up post-discovery functionality
            if (mqtt_handler) {
                // Create a method slot for the connection status change
                QObject::connect(mqtt_handler, SIGNAL(connectionStatusChanged(bool)), 
                                this, SLOT(onMQTTConnectionChanged(bool)));
                
                // Create our discovery timer
                post_discovery_timer = new QTimer(this);
                post_discovery_timer->setSingleShot(true);
                
                // Connect the timer to a lambda function
                connect(post_discovery_timer, &QTimer::timeout, this, [this]() {
                    // Protect against crashes by checking if the object is still valid
                    if (!config_manager) {
                        return;
                    }
                    
                    try {
                        // Reapply the enabled states for all discovered devices
                        auto all_devices = getAllAvailableDevices();
                        
                        // Rebuild the devices_added_to_openrgb map based on discovered devices and saved config
                        {
                            QMutexLocker locker(&device_mutex);
                            for (const auto& device_info : all_devices) {
                                const std::string& name = device_info.first;
                                bool should_be_enabled = config_manager->isDeviceEnabled(name);
                                devices_added_to_openrgb[name] = should_be_enabled;
                            }
                        }
                        
                        // Now update the device list with the refreshed map
                        // Use a queued invocation to ensure thread safety
                        QMetaObject::invokeMethod(this, "updateDeviceList", Qt::QueuedConnection);
                    }
                    catch (const std::exception& e) {
                        qWarning() << "Exception in post-discovery timer:" << e.what();
                    }
                    catch (...) {
                        qWarning() << "Unknown exception in post-discovery timer";
                    }
                });
            }
        }
    }
}

void DeviceManager::discoverAllDevices()
{
    qInfo() << "[Discovery] Starting RGB device discovery...";
    // Initialize ESPHome manager first
    if (esphome_manager) {
        esphome_manager->initializeManager();
    }
    // Then start discovery
    if (mosquitto_manager) {
        mosquitto_manager->discoverDevices();
    }
    if (zigbee_manager) {
        zigbee_manager->discoverDevices();
    }
    if (esphome_manager) {
        esphome_manager->discoverDevices();
    }
    
    // Discover DDP devices if manager is available
    if (ddp_manager) {
        ddp_manager->discoverDevices();
    }
}

void DeviceManager::handleMQTTMessage(const QString& topic, const QByteArray& payload)
{
    // Route messages to appropriate protocol manager
    if (topic.startsWith("zigbee2mqtt/")) {
        if (zigbee_manager) {
            zigbee_manager->handleMQTTMessage(topic, payload);
        }
    } else if (topic.startsWith("homeassistant/")) {
        if (mosquitto_manager) {
            mosquitto_manager->handleMQTTMessage(topic, payload);
        }
    } else if (topic.startsWith("esphome/")) {
        if (esphome_manager) {
            esphome_manager->handleMQTTMessage(topic, payload);
        }
    }
}

void DeviceManager::discoverDevices()
{
    discoverAllDevices();
}

std::vector<RGBController*> DeviceManager::getDevices() const
{
    QMutexLocker locker(&device_mutex);
    return cached_devices;
}

void DeviceManager::onMQTTConnectionChanged(bool connected)
{
    if (connected) {
        // Start the post-discovery timer when MQTT connects
        if (post_discovery_timer) {
            post_discovery_timer->start(5000);
        }
    }
}

void DeviceManager::onProtocolDevicesChanged()
{
    update_timer->start(100); // Debounce device updates
    
    // Also immediately update available devices in OpenRGB's resource manager
    try {
        if (resource_manager) {
            // This ensures OpenRGB knows about our devices immediately
            QTimer::singleShot(200, this, &DeviceManager::updateDeviceList);
        }
    } catch (...) {
        qWarning() << "Error notifying ResourceManager of device changes";
    }
}

void DeviceManager::updateDeviceList()
{
    // Check if update already in progress to prevent reentrant calls
    {
        QMutexLocker locker(&device_mutex);
        if (update_in_progress) {
            return;
        }
        update_in_progress = true;
    }
    
    try {
        // Local copies to work with
        std::vector<RGBController*> new_cached_devices;
        std::map<std::string, bool> device_status;
        
        // Make a thread-safe copy of the enabled device map
        {
            QMutexLocker locker(&device_mutex);
            device_status = devices_added_to_openrgb;
        }
        
        // Collect devices from all managers into temporary lists without holding the lock
        std::vector<RGBController*> mosquitto_devices;
        std::vector<RGBController*> zigbee_devices;
        std::vector<RGBController*> esphome_devices;
        std::vector<RGBController*> ddp_devices;
        
        // Get devices from each manager
        try {
            if (mosquitto_manager) {
                mosquitto_devices = mosquitto_manager->getDevices();
            }
        } catch (...) {
            // Silently ignore errors
        }
        
        try {
            if (zigbee_manager) {
                zigbee_devices = zigbee_manager->getDevices();
            }
        } catch (...) {
            // Silently ignore errors
        }
        
        try {
            if (esphome_manager) {
                esphome_devices = esphome_manager->getDevices();
            }
        } catch (...) {
            // Silently ignore errors
        }
        
        try {
            if (ddp_manager) {
                ddp_devices = ddp_manager->getDevices();
            }
        } catch (...) {
            // Silently ignore errors
        }
        
        // Process all devices without holding the lock for too long
        // First, create a combined list of all devices
        std::vector<RGBController*> all_devices;
        all_devices.reserve(mosquitto_devices.size() + zigbee_devices.size() + 
                           esphome_devices.size() + ddp_devices.size());
        
        all_devices.insert(all_devices.end(), mosquitto_devices.begin(), mosquitto_devices.end());
        all_devices.insert(all_devices.end(), zigbee_devices.begin(), zigbee_devices.end());
        all_devices.insert(all_devices.end(), esphome_devices.begin(), esphome_devices.end());
        all_devices.insert(all_devices.end(), ddp_devices.begin(), ddp_devices.end());
        
        // Now filter devices based on our status map
        for (auto device : all_devices) {
            if (!device) {
                continue;
            }
            
            std::string device_name = device->name;
            
            // Check if device should be added to OpenRGB
            auto it = device_status.find(device_name);
            bool should_add = false;
            
            if (it != device_status.end()) {
                should_add = it->second;
            }
            
            // Add to new cached devices list if it should be added
            if (should_add) {
                new_cached_devices.push_back(device);
            }
        }
        
        // Now safely update our cached device list with a short lock
        {
            QMutexLocker locker(&device_mutex);
            
            // We need to unregister any devices that were in the old list but not in the new list
            if (resource_manager) {
                // First, create a map of device names from the new list for quick lookup
                std::map<std::string, bool> new_device_names;
                for (auto device : new_cached_devices) {
                    new_device_names[device->name] = true;
                }
                
                // Check each device in old list
                for (auto device : cached_devices) {
                    // If a device from the old list isn't in the new list, unregister it
                    if (new_device_names.find(device->name) == new_device_names.end()) {
                        // Unregister from ResourceManager using QMetaObject::invokeMethod for thread safety
                        QMetaObject::invokeMethod(this, [this, device](){
                            if (resource_manager) {
                                resource_manager->UnregisterRGBController(device);
                            }
                        }, Qt::QueuedConnection);
                    }
                }
                
                // Now register any new devices not already registered
                for (auto device : new_cached_devices) {
                    // If this device wasn't previously registered, register it now
                    bool was_in_old_list = false;
                    for (auto old_device : cached_devices) {
                        if (old_device->name == device->name) {
                            was_in_old_list = true;
                            break;
                        }
                    }
                    
                    if (!was_in_old_list) {
                        QMetaObject::invokeMethod(this, [this, device](){
                            if (resource_manager) {
                                resource_manager->RegisterRGBController(device);
                            }
                        }, Qt::QueuedConnection);
                    }
                }
            }
            
            // Update our cache
            cached_devices.clear();
            cached_devices = new_cached_devices;
        }
        
        // Signal that the list has changed
        emit deviceListChanged();
    } catch (...) {
        // Silently ignore exceptions
    }
    
    // Always clear the in-progress flag when done
    {
        QMutexLocker locker(&device_mutex);
        update_in_progress = false;
    }
}

void DeviceManager::subscribeToTopics()
{
    // For now, we will skip subscription since we cannot easily fix the access issues
    // This function is called internally by the discovery process
    
    // We will implement a cleaner solution in a future update
    // WORKAROUND: Skip calling protected methods in protocol managers
    
    // Just let the user know what is happening
    qInfo() << "[DeviceManager] Skipping MQTT topic subscriptions (will be handled during device discovery)";
}
RGBController* DeviceManager::findDevice(const std::string& device_name)
{
    QMutexLocker locker(&device_mutex);
    for (auto device : cached_devices) {
        if (device->name == device_name) {
            return device;
        }
    }
    return nullptr;
}

bool DeviceManager::setDeviceColor(const std::string& device_name, RGBColor color)
{
    RGBController* device = findDevice(device_name);
    if (!device) {
        // Device not found
        return false;
    }

    // Set all LEDs in each zone to the specified color
    for (unsigned int zone_idx = 0; zone_idx < device->zones.size(); zone_idx++) {
        unsigned int leds_count = device->zones[zone_idx].leds_count;
        for (unsigned int led_idx = 0; led_idx < leds_count; led_idx++) {
            unsigned int global_led_idx = device->zones[zone_idx].start_idx + led_idx;
            if (global_led_idx < device->leds.size()) {
                device->colors[global_led_idx] = color;
            }
        }
        device->UpdateZoneLEDs(zone_idx);
    }

    return true;
}

bool DeviceManager::setZoneColor(const std::string& device_name, const std::string& zone_name, RGBColor color)
{
    RGBController* device = findDevice(device_name);
    if (!device) {
        qDebug() << "[DeviceManager] Device not found:" << device_name.c_str();
        return false;
    }

    for (unsigned int zone_idx = 0; zone_idx < device->zones.size(); zone_idx++) {
        if (device->zones[zone_idx].name == zone_name) {
            // Set all LEDs in this zone to the specified color
            unsigned int leds_count = device->zones[zone_idx].leds_count;
            for (unsigned int led_idx = 0; led_idx < leds_count; led_idx++) {
                unsigned int global_led_idx = device->zones[zone_idx].start_idx + led_idx;
                if (global_led_idx < device->leds.size()) {
                    device->colors[global_led_idx] = color;
                }
            }
            device->UpdateZoneLEDs(zone_idx);
            return true;
        }
    }

    // Zone not found
    return false;
}

bool DeviceManager::setLEDColor(const std::string& device_name, int led_index, RGBColor color)
{
    RGBController* device = findDevice(device_name);
    if (!device) {
        qDebug() << "[DeviceManager] Device not found:" << device_name.c_str();
        return false;
    }

    if (led_index < 0 || led_index >= (int)device->leds.size()) {
        // LED index out of range
        return false;
    }

    device->SetLED(led_index, color);
    return true;
}
bool DeviceManager::addDeviceToOpenRGB(const std::string& device_name, bool add)
{
    QMutexLocker locker(&device_mutex);
    
    try {
        // Validate inputs
        if (device_name.empty()) {
            qWarning() << "[DeviceManager] Empty device name provided";
            return false;
        }
        
        // Setting device state
        
        // Add to internal map
        devices_added_to_openrgb[device_name] = add;
        
        // Save the device state to config for persistence
        if (config_manager) {
            config_manager->setDeviceEnabled(device_name, add);
            // Device state saved to config
        }
        
        // Also register any changes with ResourceManager
        if (resource_manager) {
            // Device registration changed
            
            // We won't directly register with ResourceManager here - it can cause thread issues
            // Instead we'll let updateDeviceList handle registration safely
            // Registration will be handled during list update
        }
        
        // Schedule the device list update rather than doing it synchronously
        // This avoids potential deadlocks and UI freezes
        QTimer::singleShot(0, this, &DeviceManager::updateDeviceList);
        
        return true;
    } catch (const std::exception& e) {
        // Log specific exception using QDebug instead of LogManager to avoid linking errors
        qCritical() << "[DeviceManager] Exception in addDeviceToOpenRGB:" << e.what();
        return false;
    } catch (...) {
        // Handle any other exceptions
        qCritical() << "[DeviceManager] Unknown exception in addDeviceToOpenRGB";
        return false;
    }
}

bool DeviceManager::isDeviceAddedToOpenRGB(const std::string& device_name) const
{
    QMutexLocker locker(&device_mutex);
    
    auto it = devices_added_to_openrgb.find(device_name);
    if (it != devices_added_to_openrgb.end()) {
        return it->second;
    }
    
    // Default to not added
    return false;
}

std::vector<std::pair<std::string, std::string>> DeviceManager::getAllAvailableDevices() const
{
    QMutexLocker locker(&device_mutex);
    std::vector<std::pair<std::string, std::string>> result;
    
    // Get DDP devices
    if (ddp_manager) {
        QJsonArray devices = ddp_manager->getDeviceConfig();
        for (int i = 0; i < devices.size(); i++) {
            QJsonObject dev = devices[i].toObject();
            std::string name = dev["name"].toString().toStdString();
            result.push_back(std::make_pair(name, "DDP"));
        }
    }
    
    // Get Mosquitto devices
    if (mosquitto_manager) {
        auto devices = mosquitto_manager->getDevices();
        for (auto device : devices) {
            std::string name = device->name;
            result.push_back(std::make_pair(name, "MQTT"));
        }
    }
    
    // Get Zigbee devices
    if (zigbee_manager) {
        auto devices = zigbee_manager->getDevices();
        for (auto device : devices) {
            std::string name = device->name;
            result.push_back(std::make_pair(name, "Zigbee"));
        }
    }
    
    // ESPHome devices are currently disabled
    
    return result;
}