#include "MosquittoDeviceManager.h"
#include "MosquittoLightDevice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "OpenRGB/LogManager.h"

MosquittoDeviceManager::MosquittoDeviceManager(QObject* parent)
    : QObject(parent)
{
}

MosquittoDeviceManager::~MosquittoDeviceManager()
{
    for(auto device : devices) {
        delete device;
    }
    devices.clear();
}

void MosquittoDeviceManager::discoverDevices()
{
    // Subscribe to Home Assistant light discovery topics
    emit mqttPublishNeeded("homeassistant/light/#", "");
    emit mqttPublishNeeded("homeassistant/+/light/+/config", "");
}

void MosquittoDeviceManager::handleMQTTMessage(const QString& topic, const QByteArray& payload)
{
    // Handle Home Assistant discovery messages
    if (topic.startsWith("homeassistant/light") && topic.endsWith("/config")) {
        processDeviceConfig(topic, payload);
    }
}

std::vector<RGBController*> MosquittoDeviceManager::getDevices() const
{
    std::vector<RGBController*> result;
    result.reserve(devices.size());
    for(auto device : devices) {
        result.push_back(device);
    }
    return result;
}

void MosquittoDeviceManager::processDeviceConfig(const QString& topic, const QByteArray& payload)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return;

    // Process device config

    QJsonObject config = doc.object();
    
    // Get the base topic if available
    QString baseTopic;
    if (config.contains("~")) {
        baseTopic = config.value("~").toString();
    }

    // Extract device information
    MQTTRGBDevice::LightInfo info;
    QJsonObject deviceObj = config.value("dev").toObject();
    info.name = deviceObj.value("name").toString();
    if (info.name.isEmpty()) {
        info.name = config.value("name").toString();
    }
    LOG_DEBUG("Device name set to: %s", qUtf8Printable(info.name));
    
    // Extract all required MQTT topics
    info.command_topic = config.value("rgb_cmd_t").toString();
    info.state_topic = config.value("rgb_stat_t").toString();
    // Replace ~/ with base topic if needed
    if (!baseTopic.isEmpty() && info.state_topic.startsWith("~/")) {
        info.state_topic = baseTopic + "/" + info.state_topic.mid(2);
    }
    
    info.unique_id = config.value("uniq_id").toString();
    info.rgb_command_template = config.value("rgb_cmd_tpl").toString();
    info.rgb_value_template = config.value("rgb_val_tpl").toString();
    
    // Replace ~ with base topic if needed
    if (!baseTopic.isEmpty()) {
        if (info.command_topic.startsWith("~/")) {
            info.command_topic = info.command_topic.mid(2);
            info.command_topic = baseTopic + "/" + info.command_topic;
        }
        if (info.state_topic.startsWith("~/")) {
            info.state_topic = info.state_topic.mid(2);
            info.state_topic = baseTopic + "/" + info.state_topic;
        }
    }
    
    info.rgb_command_template = config.value("rgb_cmd_tpl").toString();
    info.rgb_value_template = config.value("rgb_val_tpl").toString();
    
    if (info.name.isEmpty() || info.command_topic.isEmpty())
        return;

    info.has_brightness = !config.value("bri_cmd_t").toString().isEmpty();
    info.has_rgb = true;
    info.num_leds = 1;

    QString deviceTopic = topic.left(topic.lastIndexOf("/"));
    auto it = devices.find(deviceTopic);
    if (it == devices.end()) {
        MosquittoLightDevice* device = new MosquittoLightDevice(info);
        connect(device, &MosquittoLightDevice::mqttPublishNeeded,
                this, &MosquittoDeviceManager::mqttPublishNeeded);
        devices[deviceTopic] = device;
        emit deviceListChanged();
    }
}

void MosquittoDeviceManager::processDeviceState(const QString& topic, const QByteArray& payload)
{
    auto it = devices.find(topic);
    if (it != devices.end()) {
        it.value()->UpdateFromMQTT(payload);
        emit deviceListChanged();
    }
}

void MosquittoDeviceManager::subscribeToTopics()
{
    // No additional subscriptions needed - handled in discoverDevices()
}