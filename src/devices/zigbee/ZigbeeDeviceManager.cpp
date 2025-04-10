#include "ZigbeeDeviceManager.h"
#include "ZigbeeLightDevice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "OpenRGB/LogManager.h"
#include <QMutexLocker>

ZigbeeDeviceManager::ZigbeeDeviceManager(QObject* parent)
    : QObject(parent)
{
    // Send a test message as soon as the manager is created
    QTimer::singleShot(3000, this, [this]() {
        // Create a test message to turn the "Worm" light blue
        QJsonObject color;
        color["x"] = 0.1691;
        color["y"] = 0.0441;
        
        QJsonObject payload;
        payload["state"] = "ON";
        payload["color"] = color;
        
        QJsonDocument doc(payload);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        
        // Send directly to the zigbee2mqtt topic
        LOG_INFO("[ZigbeeDeviceManager] Sending test message to Worm light");
        emit mqttPublishNeeded("zigbee2mqtt/Worm/set", data);
    });
}

ZigbeeDeviceManager::~ZigbeeDeviceManager()
{
    QMutexLocker locker(&device_mutex);
    for(auto device : devices) {
        delete device;
    }
    devices.clear();
}

void ZigbeeDeviceManager::discoverDevices()
{
    LOG_INFO("[ZigbeeDeviceManager] Starting Zigbee device discovery");
    
    // Subscribe to bridge state and response topics
    // Only emit if bridge state unknown
    if (!bridge_state_known) {
        emit mqttPublishNeeded("zigbee2mqtt/bridge/state", QByteArray());
    }
    
    // Request device list
    QJsonObject request;
    request["topic"] = "bridge/devices";
    QJsonDocument doc(request);
    emit mqttPublishNeeded("zigbee2mqtt/bridge/request/devices", doc.toJson(QJsonDocument::Compact));
    
    // Send a simple test message to Worm device
    QTimer::singleShot(2000, this, [this]() {
        QByteArray payload = "{\"state\":\"ON\"}";
        LOG_INFO("[ZigbeeDeviceManager] Sending direct test ON message to Worm");
        emit mqttPublishNeeded("zigbee2mqtt/Worm/set", payload);
    });
}

bool ZigbeeDeviceManager::isRGBLight(const QJsonObject& device) const
{
    if (!device.contains("definition"))
        return false;

    QJsonObject definition = device["definition"].toObject();
    if (!definition.contains("exposes"))
        return false;

    QJsonArray features = definition["exposes"].toArray();
    for (const QJsonValue& feature : features) {
        QJsonObject featureObj = feature.toObject();
        if (featureObj["type"].toString() != "light")
            continue;

        // Check for specific features that indicate RGB capability
        QJsonArray subFeatures = featureObj["features"].toArray();
        for (const QJsonValue& subFeature : subFeatures) {
            QString property = subFeature.toObject()["property"].toString();
            if (property == "color_xy" || property == "color_hs" || 
                property == "color_rgb" || property == "color") {
                return true;
            }
        }
    }
    return false;
}

void ZigbeeDeviceManager::handleMQTTMessage(const QString& topic, const QByteArray& payload)
{
    QMutexLocker locker(&device_mutex);
    
    // Handle bridge state
    if (topic == "zigbee2mqtt/bridge/state") {
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        QString state = doc.object()["state"].toString();
        
        if (state == "online" && !bridge_state_known) {
            bridge_state_known = true;
            // Request initial device list
            QJsonObject request;
            request["topic"] = "bridge/devices";
            QJsonDocument doc(request);
            emit mqttPublishNeeded("zigbee2mqtt/bridge/request/devices", doc.toJson(QJsonDocument::Compact));
        }
        return;
    }

    // Handle device list response
    if (topic == "zigbee2mqtt/bridge/devices" || topic == "zigbee2mqtt/bridge/response/devices") {
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (!doc.isArray())
            return;
            
        QJsonArray deviceList = doc.array();
        // Process zigbee devices
        
        // Unsubscribe from all previous topics
        QMapIterator<QString, ZigbeeLightDevice*> deviceIter(devices);
        while (deviceIter.hasNext()) {
            deviceIter.next();
            emit mqttPublishNeeded(deviceIter.key(), QByteArray());  // Empty payload for unsubscribe
        }
        
        for (const QJsonValue& deviceVal : deviceList) {
            QJsonObject device = deviceVal.toObject();
            
            // Skip if not an RGB light
            if (!isRGBLight(device)) {
                continue;
            }
                
            QString friendly_name = device["friendly_name"].toString();
            QString deviceTopic = "zigbee2mqtt/" + friendly_name;
            
            // Found zigbee RGB light - device creation handled in DeviceManager
            
            // Create device if it doesn't exist
            if (!devices.contains(deviceTopic)) {
                MQTTRGBDevice::LightInfo info;
                info.name = friendly_name;
                info.unique_id = device["ieee_address"].toString();
                info.state_topic = deviceTopic;
                info.command_topic = deviceTopic + "/set";
                info.num_leds = 1;
                info.has_rgb = true;
                info.has_brightness = true;
                
                LOG_INFO("Creating ZigbeeLightDevice: %s, topic: %s, command topic: %s", 
                     qUtf8Printable(friendly_name), 
                     qUtf8Printable(deviceTopic), 
                     qUtf8Printable(deviceTopic + "/set"));
                
                ZigbeeLightDevice* newDevice = new ZigbeeLightDevice(info);
                
                // Connect both signals - use direct string-based SIGNAL/SLOT for more reliable connection
                bool connection1 = connect(newDevice, SIGNAL(publishMessage(QString,QByteArray)),
                                        this, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
                                        Qt::DirectConnection);
                                        
                bool connection2 = connect(newDevice, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
                                        this, SIGNAL(mqttPublishNeeded(QString,QByteArray)),
                                        Qt::DirectConnection);
                
                LOG_INFO("Signal connections: publishMessage=%s, mqttPublishNeeded=%s",
                         connection1 ? "SUCCESS" : "FAILED",
                         connection2 ? "SUCCESS" : "FAILED");
                
                // Test the connection by sending a direct message
                QTimer::singleShot(1000, this, [this, friendly_name]() {
                    QByteArray testMsg = "{\"state\":\"ON\"}";
                    QString testTopic = QString("zigbee2mqtt/%1/set").arg(friendly_name);
                    LOG_INFO("[ZigbeeDeviceManager] Testing message to: %s", qUtf8Printable(testTopic));
                    emit mqttPublishNeeded(testTopic, testMsg);
                });
                
                devices[deviceTopic] = newDevice;
                
                // Subscribe only to this device's state topic
                emit mqttPublishNeeded(deviceTopic, QByteArray());  // Empty payload for subscribe
            } else {
                LOG_DEBUG("Zigbee device already exists: %s", qUtf8Printable(friendly_name));
            }
        }
        
        emit deviceListChanged();
        return;
    }

    // Only process messages for known RGB light devices
    if (devices.contains(topic)) {
        devices[topic]->UpdateFromMQTT(payload);
        emit deviceListChanged();
    }
}

std::vector<RGBController*> ZigbeeDeviceManager::getDevices() const
{
    QMutexLocker locker(&device_mutex);
    std::vector<RGBController*> result;
    result.reserve(devices.size());
    for(auto device : devices) {
        result.push_back(device);
    }
    return result;
}

void ZigbeeDeviceManager::subscribeToTopics()
{
    // Already handled in discoverDevices()
}