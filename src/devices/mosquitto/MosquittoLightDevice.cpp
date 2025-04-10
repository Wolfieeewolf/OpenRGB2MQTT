#include "MosquittoLightDevice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "OpenRGB/LogManager.h"

MosquittoLightDevice::MosquittoLightDevice(const LightInfo& info)
    : MQTTRGBDevice(info)
{
}

MosquittoLightDevice::~MosquittoLightDevice()
{
}

void MosquittoLightDevice::UpdateFromMQTT(const QByteArray& payload)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return;

    QJsonObject state = doc.object();
    
    // Check if light is on
    bool is_on = (state["state"].toString().toUpper() != "OFF");
    
    if (!is_on) {
        for(unsigned int i = 0; i < colors.size(); i++) {
            colors[i] = 0x000000;
        }
        return;
    }

    // Handle standard color format
    RGBColor new_color = 0x000000;
    if (state.contains("color")) {
        QJsonObject color = state["color"].toObject();
        if (color.contains("r") && color.contains("g") && color.contains("b")) {
            unsigned char r = static_cast<unsigned char>(color["r"].toInt());
            unsigned char g = static_cast<unsigned char>(color["g"].toInt());
            unsigned char b = static_cast<unsigned char>(color["b"].toInt());
            new_color = ToRGBColor(r, g, b);
        }
    }
    
    // Set the color
    if (colors.size() == 1) {
        colors[0] = new_color;
    }
}

void MosquittoLightDevice::PublishState()
{
    if (colors.empty()) return;

    QJsonObject state;
    state["state"] = "ON";

    QJsonObject color;
    color["r"] = static_cast<int>(RGBGetRValue(colors[0]));
    color["g"] = static_cast<int>(RGBGetGValue(colors[0]));
    color["b"] = static_cast<int>(RGBGetBValue(colors[0]));
    state["color"] = color;

    QJsonDocument doc(state);
    emit mqttPublishNeeded(mqtt_topic, doc.toJson(QJsonDocument::Compact));
}