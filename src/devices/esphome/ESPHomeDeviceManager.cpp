#include "ESPHomeDeviceManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ESPHomeDeviceManager::ESPHomeDeviceManager(QObject* parent)
    : QObject(parent)
    , api_manager(nullptr)
    , initialization_complete(false)
{
    qDebug() << "[ESPHome] Initializing device manager";
}

void ESPHomeDeviceManager::initializeManager()
{
    QMutexLocker locker(&device_mutex);
    if (api_manager == nullptr) {
        try {
            api_manager = new ESPHomeAPIManager(this);
            connect(api_manager, &ESPHomeAPIManager::deviceListChanged,
                    this, [this]() {
                        QMetaObject::invokeMethod(this, "deviceListChanged", Qt::QueuedConnection);
                    }, Qt::DirectConnection);
            qDebug() << "[ESPHome] API manager initialized";
            
            // Delay marking initialization complete
            QTimer::singleShot(1000, this, [this]() {
                QMutexLocker initLocker(&device_mutex);
                initialization_complete = true;
                qDebug() << "[ESPHome] Initialization complete";
            });
        } catch (const std::exception& e) {
            qDebug() << "[ESPHome] Failed to initialize API manager:" << e.what();
            if (api_manager) {
                delete api_manager;
                api_manager = nullptr;
            }
        }
    }
}

ESPHomeDeviceManager::~ESPHomeDeviceManager()
{
    qDebug() << "[ESPHome] Cleaning up device manager";
    // Clean up MQTT devices
    for(const auto& device : mqtt_devices.values()) {
        delete device;
    }
    mqtt_devices.clear();

    // Clean up API manager
    if (api_manager) {
        delete api_manager;
        api_manager = nullptr;
    }
    qDebug() << "[ESPHome] Cleanup complete";
}

void ESPHomeDeviceManager::subscribeToTopics()
{
    // Subscribe to ESPHome discovery and state topics
    emit mqttPublishNeeded("esphome/+/light/+/config", QByteArray());
    emit mqttPublishNeeded("esphome/+/light/+/state", QByteArray());
}

void ESPHomeDeviceManager::discoverDevices()
{
    qDebug() << "[ESPHome] Starting device discovery";
    // Start MQTT discovery
    subscribeToTopics();
    
    // Start native API discovery
    if (api_manager) {
        api_manager->startDiscovery();
    } else {
        qDebug() << "[ESPHome] API manager not initialized, skipping API discovery";
    }
}

std::vector<RGBController*> ESPHomeDeviceManager::getDevices() const
{
    std::vector<RGBController*> result;
    QMutexLocker locker(&device_mutex);  // Thread-safe access
    
    if (!initialization_complete) {
        qDebug() << "[ESPHome] Initialization not complete, returning empty device list";
        return result;
    }

    try {
        // Get MQTT devices
        result.reserve(mqtt_devices.size());
        for (auto device : mqtt_devices) {
            if (device && device->isInitialized()) {  // Check both pointer and initialization
                result.push_back(device);
            }
        }
        
        // Get API devices
        if (api_manager) {
        auto api_devices = api_manager->getDevices();
        for (auto device : api_devices) {
        if (device && device->vendor == "ESPHome") {
        if (auto* esphome_device = dynamic_cast<ESPHomeAPIDevice*>(device)) {
                if (esphome_device->isInitialized()) {
                        result.push_back(device);
                        }
                        }
                    } else if (device) { // Non-ESPHome devices
                        result.push_back(device);
                    }
                }
            }
    } catch (const std::exception& e) {
        qDebug() << "[ESPHome] Error getting devices:" << e.what();
    }
    
    return result;
}

void ESPHomeDeviceManager::handleMQTTMessage(const QString& topic, const QByteArray& payload)
{
    QStringList parts = topic.split('/');
    
    // Expect format: esphome/devicename/light/name/(config|state)
    if (parts.size() != 5) {
        return;
    }

    QString deviceName = parts[1];
    QString lightName = parts[3];
    QString messageType = parts[4];
    QString deviceTopic = QString("esphome/%1/light/%2").arg(deviceName, lightName);

    if (messageType == "config") {
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (!doc.isObject()) {
            return;
        }

        QJsonObject config = doc.object();
        if (processDeviceConfig(deviceName, lightName, config)) {
            emit deviceListChanged();
        }
    }
    else if (messageType == "state") {
        auto it = mqtt_devices.find(deviceTopic);
        if (it != mqtt_devices.end()) {
            it.value()->UpdateFromMQTT(payload);
        }
    }
}

bool ESPHomeDeviceManager::processDeviceConfig(const QString& deviceName, 
                                             const QString& lightName,
                                             const QJsonObject& config)
{
    if (!isValidRGBLight(config)) {
        return false;
    }

    // Create device info
    MQTTRGBDevice::LightInfo info;
    
    info.name = QString("%1_%2").arg(deviceName, lightName);
    info.unique_id = config["unique_id"].toString();
    info.state_topic = QString("esphome/%1/light/%2/state").arg(deviceName, lightName);
    info.command_topic = QString("esphome/%1/light/%2/command").arg(deviceName, lightName);
    
    // Get supported features
    info.has_brightness = true;
    info.has_rgb = true;
    
    // Get effects if available
    if (config.contains("effects")) {
        QJsonArray effectsList = config["effects"].toArray();
        for (const auto& effect : effectsList) {
            info.effect_list.append(effect.toString());
        }
    }

    QString deviceTopic = QString("esphome/%1/light/%2").arg(deviceName, lightName);

    // Update or create device
    auto it = mqtt_devices.find(deviceTopic);
    if (it != mqtt_devices.end()) {
        // Update existing device
        (*it)->UpdateConfiguration(info);
        return false;
    } else {
        // Create new device
        ESPHomeLightDevice* device = new ESPHomeLightDevice(info);
        connect(device, &ESPHomeLightDevice::mqttPublishNeeded,
                this, &ESPHomeDeviceManager::mqttPublishNeeded);
        mqtt_devices[deviceTopic] = device;
        return true;
    }
}

bool ESPHomeDeviceManager::isValidRGBLight(const QJsonObject& config) const
{
    // Check for RGB support via supported modes
    if (config.contains("supported_color_modes")) {
        QJsonArray modes = config["supported_color_modes"].toArray();
        return modes.contains(35);  // Mode 35 is RGB in ESPHome
    }
    
    // Fallback check for legacy RGB support
    return config["legacy_supports_rgb"].toBool(false);
}

void ESPHomeDeviceManager::cleanupDevice(const QString& topic)
{
    auto it = mqtt_devices.find(topic);
    if (it != mqtt_devices.end()) {
        delete it.value();
        mqtt_devices.remove(topic);
    }
}