#include "ESPHomeLightDevice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ESPHomeLightDevice::ESPHomeLightDevice(const LightInfo& info)
    : MQTTRGBDevice(info)
    , current_effect(0)
    , brightness_support(info.has_brightness)
    , initialized(false)
{
    name = info.name.toStdString();
    vendor = "ESPHome";
    type = DEVICE_TYPE_LEDSTRIP;
    description = "ESPHome RGB Device";
    version = "1.0";
    serial = info.unique_id.toStdString();
    location = "ESPHome";

    // Set up modes
    modes.resize(1);
    modes[0].name = "Direct";
    modes[0].value = 0;
    modes[0].flags = MODE_FLAG_HAS_PER_LED_COLOR;
    if (brightness_support) {
        modes[0].flags |= MODE_FLAG_HAS_BRIGHTNESS;
    }
    modes[0].color_mode = MODE_COLORS_PER_LED;

    // Copy effect list
    effect_list = info.effect_list;

    SetupZones();
    
    active_mode = 0;
    initialized = true;
}

ESPHomeLightDevice::~ESPHomeLightDevice()
{
}

void ESPHomeLightDevice::UpdateConfiguration(const LightInfo& new_info)
{
    brightness_support = new_info.has_brightness;
    effect_list = new_info.effect_list;
    
    // Update mode flags if needed
    if (brightness_support) {
        modes[0].flags |= MODE_FLAG_HAS_BRIGHTNESS;
    }
}

void ESPHomeLightDevice::SetupZones()
{
    zones.resize(1);
    zones[0].name = "Main";
    zones[0].type = ZONE_TYPE_SINGLE;
    zones[0].leds_min = 1;
    zones[0].leds_max = 1;
    zones[0].leds_count = 1;
    zones[0].matrix_map = NULL;

    leds.resize(1);
    leds[0].name = "Main LED";
    
    colors.resize(1);
    colors[0] = 0;
}

void ESPHomeLightDevice::UpdateFromMQTT(const QByteArray& payload)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject state = doc.object();
    
    // Parse state
    bool is_on = (state["state"].toString().toUpper() == "ON");
    
    if (!is_on) {
        colors[0] = 0x000000;
        return;
    }

    // Parse RGB
    if (state.contains("rgb")) {
        QJsonArray rgb = state["rgb"].toArray();
        if (rgb.size() == 3) {
            unsigned char r = rgb[0].toInt();
            unsigned char g = rgb[1].toInt();
            unsigned char b = rgb[2].toInt();
            colors[0] = ToRGBColor(r, g, b);
        }
    }

    // Parse brightness
    if (brightness_support && state.contains("brightness")) {
        int brightness = state["brightness"].toInt();
        for (auto& mode : modes) {
            if (mode.flags & MODE_FLAG_HAS_BRIGHTNESS) {
                mode.brightness = (brightness * 100) / 255;  // Convert 0-255 to percentage
            }
        }
    }

    // Parse effect if present
    if (state.contains("effect")) {
        QString effect = state["effect"].toString();
        int index = effect_list.indexOf(effect);
        if (index >= 0) {
            current_effect = index;
        }
    }
}

void ESPHomeLightDevice::PublishState()
{
    QJsonObject state;
    
    state["state"] = (colors[0] == 0x000000) ? "OFF" : "ON";

    if (colors[0] != 0x000000) {
        // RGB values
        QJsonArray rgb;
        rgb.append(static_cast<int>(RGBGetRValue(colors[0])));
        rgb.append(static_cast<int>(RGBGetGValue(colors[0])));
        rgb.append(static_cast<int>(RGBGetBValue(colors[0])));
        state["rgb"] = rgb;

        // Set brightness based on max RGB value
        if (brightness_support) {
            unsigned char max_component = std::max(std::max(
                RGBGetRValue(colors[0]),
                RGBGetGValue(colors[0])),
                RGBGetBValue(colors[0])
            );
            state["brightness"] = static_cast<int>(max_component);
        }

        // Set effect if applicable
        if (current_effect >= 0 && current_effect < effect_list.size()) {
            state["effect"] = effect_list[current_effect];
        }
    }

    QJsonDocument doc(state);
    emit mqttPublishNeeded(mqtt_topic + "/command", doc.toJson(QJsonDocument::Compact));
}

void ESPHomeLightDevice::DeviceUpdateLEDs()
{
    PublishState();
}

void ESPHomeLightDevice::UpdateZoneLEDs(int /*zone*/)
{
    PublishState();
}

void ESPHomeLightDevice::UpdateSingleLED(int /*led*/)
{
    PublishState();
}

void ESPHomeLightDevice::SetCustomMode()
{
    active_mode = 0;
    current_effect = 0;  // Reset to no effect
}

void ESPHomeLightDevice::DeviceUpdateMode()
{
    // Not implemented - single mode only
}