#include "DeviceManager.h"
#include "mosquitto/MosquittoDeviceManager.h"
#include "zigbee/ZigbeeDeviceManager.h"
#include "esphome/ESPHomeDeviceManager.h"
#include <QDebug>

DeviceManager::DeviceManager(ResourceManagerInterface* resource_manager, QObject* parent)
    : QObject(parent)
    , resource_manager(resource_manager)
    , mosquitto_manager(new MosquittoDeviceManager(this))
    , zigbee_manager(new ZigbeeDeviceManager(this))
    , esphome_manager(new ESPHomeDeviceManager(this))
    , update_timer(new QTimer(this))
{
    update_timer->setSingleShot(true);
    connect(update_timer, &QTimer::timeout, this, &DeviceManager::updateDeviceList);

    // Initialize device manager

    // Connect protocol manager signals
    connect(mosquitto_manager, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
            this, SIGNAL(mqttPublishNeeded(QString,QByteArray)));
    connect(mosquitto_manager, SIGNAL(deviceListChanged()),
            this, SLOT(onProtocolDevicesChanged()));

    connect(zigbee_manager, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
            this, SIGNAL(mqttPublishNeeded(QString,QByteArray)));
    connect(zigbee_manager, SIGNAL(deviceListChanged()),
            this, SLOT(onProtocolDevicesChanged()));

    connect(esphome_manager, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
            this, SIGNAL(mqttPublishNeeded(QString,QByteArray)));
    connect(esphome_manager, SIGNAL(deviceListChanged()),
            this, SLOT(onProtocolDevicesChanged()));
}

DeviceManager::~DeviceManager()
{
    delete mosquitto_manager;
    delete zigbee_manager;
    delete esphome_manager;
    delete update_timer;
}

void DeviceManager::discoverAllDevices()
{
    qInfo() << "[Discovery] Starting RGB device discovery...";
    // Initialize ESPHome manager first
    if (esphome_manager) {
        esphome_manager->initializeManager();
    }
    // Then start discovery
    mosquitto_manager->discoverDevices();
    zigbee_manager->discoverDevices();
    esphome_manager->discoverDevices();
}

void DeviceManager::handleMQTTMessage(const QString& topic, const QByteArray& payload)
{
    // Route messages to appropriate protocol manager
    if (topic.startsWith("zigbee2mqtt/")) {
        zigbee_manager->handleMQTTMessage(topic, payload);
    } else if (topic.startsWith("homeassistant/")) {
        mosquitto_manager->handleMQTTMessage(topic, payload);
    } else if (topic.startsWith("esphome/")) {
        esphome_manager->handleMQTTMessage(topic, payload);
    }
}

void DeviceManager::onProtocolDevicesChanged()
{
    // Avoid rapid repeated updates
    if (!update_timer->isActive()) {
        update_timer->start(100); // 100ms debounce
    }
}

void DeviceManager::registerDevice(RGBController* device)
{
    if (!resource_manager) {
        qWarning() << "[Error] Resource manager is null!";
        return;
    }
    
    qInfo() << "[Device] Added:" << QString::fromStdString(device->name);
    
    // Get current controller list
    std::vector<RGBController*>& controllers = resource_manager->GetRGBControllers();
    controllers.push_back(device);
    
    // Tell OpenRGB to refresh
    resource_manager->UpdateDeviceList();
}

void DeviceManager::updateDeviceList()
{
    QMutexLocker locker(&device_mutex);
    
    if (!resource_manager) {
        qWarning() << "[Error] Resource manager is null!";
        return;
    }
    
    try {
        // Clear cached devices
        cached_devices.clear();

        // Get devices from each protocol with safety checks
        std::vector<RGBController*> zigbee_devices;
        std::vector<RGBController*> mosquitto_devices;
        std::vector<RGBController*> esphome_devices;

        if (zigbee_manager) zigbee_devices = zigbee_manager->getDevices();
        if (mosquitto_manager) mosquitto_devices = mosquitto_manager->getDevices();
        if (esphome_manager) {
            try {
                esphome_devices = esphome_manager->getDevices();
            } catch (const std::exception& e) {
                qDebug() << "[ESPHome] Error getting devices:" << e.what();
            }
        }

        // Log device discovery
        if (!zigbee_devices.empty() || !mosquitto_devices.empty() || !esphome_devices.empty()) {
            qInfo() << "\n[Devices] RGB devices found:";
        }

        // Add devices with validation
        auto addDevices = [this](const std::vector<RGBController*>& devices, const QString& type) {
            if (!devices.empty()) {
                qInfo() << '[' << type << "]";
                for (auto device : devices) {
                    if (device) {
                        try {
                            qInfo() << "- Found:" << QString::fromStdString(device->name);
                            cached_devices.push_back(device);
                        } catch (const std::exception& e) {
                            qDebug() << "[" << type << "] Error adding device:" << e.what();
                        }
                    }
                }
            }
        };

        addDevices(zigbee_devices, "Zigbee");
        addDevices(mosquitto_devices, "MQTT");
        addDevices(esphome_devices, "ESPHome");

        if (!cached_devices.empty()) {
            qInfo() << "\nTotal RGB devices found:" << cached_devices.size();
        }

        // Update OpenRGB's controller list safely using a delayed invocation
        QTimer::singleShot(100, this, [this]() {
            QMutexLocker lock(&device_mutex);
            
            try {
                if (!resource_manager) return;
                
                std::vector<RGBController*>& controllers = resource_manager->GetRGBControllers();
                
                // Safely remove old devices
                auto newEnd = std::remove_if(controllers.begin(), controllers.end(),
                    [](RGBController* controller) {
                        if (!controller) return true;
                        return controller->vendor == "MQTT" ||
                               controller->vendor == "ESPHome" ||
                               controller->vendor == "Zigbee";
                    });
                controllers.erase(newEnd, controllers.end());
                
                // Add validated devices
                for (auto device : cached_devices) {
                    if (device) {
                        controllers.push_back(device);
                    }
                }
                
                // Update OpenRGB
                resource_manager->UpdateDeviceList();
                emit deviceListChanged();
                
            } catch (const std::exception& e) {
                qDebug() << "[DeviceManager] Error updating device list:" << e.what();
            }
        });
        
    } catch (const std::exception& e) {
        qDebug() << "[DeviceManager] Critical error in updateDeviceList:" << e.what();
    }
}

void DeviceManager::discoverDevices()
{
    // Base implementation, protocol-specific managers should override this
}

std::vector<RGBController*> DeviceManager::getDevices() const
{
    // Base implementation just returns empty vector, protocol-specific managers override this
    return std::vector<RGBController*>();
}

void DeviceManager::subscribeToTopics()
{
    // Base implementation, protocol-specific managers should override this
}

bool DeviceManager::setDeviceColor(const std::string& device_name, RGBColor color)
{
    RGBController* device = findDevice(device_name);
    if (!device)
        return false;

    // Set all LEDs to the specified color
    for (unsigned int i = 0; i < device->colors.size(); i++) {
        device->colors[i] = color;
    }

    device->DeviceUpdateLEDs();
    emit deviceColorChanged(device_name, color);
    return true;
}

bool DeviceManager::setZoneColor(const std::string& device_name, const std::string& zone_name, RGBColor color)
{
    RGBController* device = findDevice(device_name);
    if (!device)
        return false;

    // Find matching zone
    for (unsigned int i = 0; i < device->zones.size(); i++) {
        if (device->zones[i].name == zone_name) {
            // Set all LEDs in zone to color
            for (unsigned int j = 0; j < device->zones[i].leds_count; j++) {
                device->colors[device->zones[i].start_idx + j] = color;
            }
            device->UpdateZoneLEDs(i);
            emit deviceColorChanged(device_name, color);
            return true;
        }
    }

    return false;
}

bool DeviceManager::setLEDColor(const std::string& device_name, int led_index, RGBColor color)
{
    RGBController* device = findDevice(device_name);
    if (!device || led_index < 0 || led_index >= static_cast<int>(device->colors.size()))
        return false;

    device->colors[led_index] = color;
    device->UpdateSingleLED(led_index);
    emit deviceColorChanged(device_name, color);
    return true;
}

RGBController* DeviceManager::findDevice(const std::string& device_name)
{
    if (!resource_manager)
        return nullptr;

    std::vector<RGBController*>& controllers = resource_manager->GetRGBControllers();
    for (RGBController* controller : controllers) {
        if (controller->name == device_name)
            return controller;
    }

    return nullptr;
}