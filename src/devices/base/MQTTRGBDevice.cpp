#include "MQTTRGBDevice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

MQTTRGBDevice::MQTTRGBDevice(const LightInfo& info)
    : mqtt_topic(info.command_topic)
    , rgb_command_template(info.rgb_command_template)
    , rgb_value_template(info.rgb_value_template)
    , send_updates(true)
    , color_mode(MODE_COLORS_PER_LED)
{
    name        = info.name.toStdString();
    vendor      = "MQTT";
    type        = DEVICE_TYPE_LEDSTRIP;
    description = "MQTT RGB Device";
    version     = "1.0";
    serial      = info.unique_id.toStdString();
    location    = "MQTT";

    // Set up modes
    modes.resize(1 + (info.has_effects ? info.effect_list.size() : 0));

    // Direct mode
    modes[0].name       = "Direct";
    modes[0].value      = 0;
    modes[0].flags      = MODE_FLAG_HAS_PER_LED_COLOR;
    if (info.has_brightness)
        modes[0].flags |= MODE_FLAG_HAS_BRIGHTNESS;
    modes[0].color_mode = MODE_COLORS_PER_LED;
    modes[0].speed_min  = 0;
    modes[0].speed_max  = 100;
    modes[0].colors_min = 0;
    modes[0].colors_max = info.num_leds;

    // Add effect modes if supported
    if (info.has_effects)
    {
        for(int i = 0; i < info.effect_list.size(); i++)
        {
            modes[i + 1].name       = info.effect_list[i].toStdString();
            modes[i + 1].value      = i + 1;
            modes[i + 1].flags      = 0;
            if (info.has_brightness)
                modes[i + 1].flags |= MODE_FLAG_HAS_BRIGHTNESS;
            modes[i + 1].color_mode = MODE_COLORS_NONE;
        }
    }

    SetupZones();

    // Set as custom mode by default
    active_mode = 0;
}

MQTTRGBDevice::~MQTTRGBDevice()
{
}

void MQTTRGBDevice::SetupZones()
{
    zones.resize(1);
    zones[0].name       = "Main Zone";
    zones[0].type       = ZONE_TYPE_LINEAR;
    zones[0].leds_min   = modes[0].colors_max;
    zones[0].leds_max   = modes[0].colors_max;
    zones[0].leds_count = modes[0].colors_max;
    zones[0].matrix_map = NULL;

    leds.resize(zones[0].leds_count);
    colors.resize(zones[0].leds_count);

    for(unsigned int led_idx = 0; led_idx < zones[0].leds_count; led_idx++)
    {
        leds[led_idx].name = "LED " + std::to_string(led_idx);
    }

    SetupColors();
}

void MQTTRGBDevice::DeviceUpdateLEDs()
{
    if (!send_updates)
        return;

    // Send colors directly to MQTT
    if (colors.size() == 1) {
        // Get RGB values
        int r = static_cast<int>(RGBGetRValue(colors[0]));
        int g = static_cast<int>(RGBGetGValue(colors[0]));
        int b = static_cast<int>(RGBGetBValue(colors[0]));

        // The template is: "{{'#%02x%02x%02x0000'|format(red,green,blue)}}"
        // We'll just send the format without the template syntax
        QString payload = QString::asprintf("#%02x%02x%02x0000", r, g, b);
        
        qDebug() << "Sending color values - R:" << r << "G:" << g << "B:" << b;
        qDebug() << "Sending MQTT payload:" << payload << "to topic:" << mqtt_topic;
        emit mqttPublishNeeded(mqtt_topic, payload.toUtf8());
    }
    // Multiple LED support would go here if needed
}

void MQTTRGBDevice::UpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void MQTTRGBDevice::UpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void MQTTRGBDevice::SetCustomMode()
{
    active_mode = 0;
    modes[active_mode].color_mode = color_mode;
}

void MQTTRGBDevice::DeviceUpdateMode()
{
    if (active_mode == 0)
    {
        DeviceUpdateLEDs();
    }
}

void MQTTRGBDevice::UpdateMode()
{
    DeviceUpdateMode();
}

void MQTTRGBDevice::ResizeZone(int /*zone*/, int /*new_size*/)
{
    // Not implemented - fixed size
}

void MQTTRGBDevice::PublishState()
{
    DeviceUpdateLEDs();
}

void MQTTRGBDevice::UpdateFromMQTT(const QByteArray& payload)
{
    send_updates = false;

    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return;

    QJsonObject state = doc.object();
    
    // Check if light is on
    bool is_on = (state["state"].toString().toUpper() != "OFF");
    
    // If light is off, set all LEDs to black
    if (!is_on) {
        for(unsigned int i = 0; i < colors.size(); i++) {
            colors[i] = 0x000000;
        }
        send_updates = true;
        return;
    }

    // Handle brightness
    int brightness = 100;
    if (state.contains("brightness")) {
        brightness = (state["brightness"].toInt() * 100) / 255; // Convert 0-255 to 0-100
        for (auto& mode : modes) {
            if (mode.flags & MODE_FLAG_HAS_BRIGHTNESS) {
                mode.brightness = brightness;
            }
        }
    }

    // Handle effects
    if (state.contains("effect")) {
        QString effect = state["effect"].toString();
        // Find matching effect mode
        for(unsigned int i = 1; i < modes.size(); i++) {
            if (QString::fromStdString(modes[i].name) == effect) {
                active_mode = i;
                break;
            }
        }
    } else {
        active_mode = 0; // Direct mode
    }

    // If we have a color value template, use it
    if (!rgb_value_template.isEmpty() && state.contains("hex")) {
        QString hex = state["hex"].toString();
        // Parse hex string according to template
        // TODO: Implement hex parsing based on template
    }
    // Otherwise try standard formats
    else {
        RGBColor new_color = 0x000000;
        
        // Format 1: {"color": {"r": 255, "g": 0, "b": 0}}
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

    send_updates = true;
}