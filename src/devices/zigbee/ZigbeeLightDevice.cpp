#include "ZigbeeLightDevice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <cmath>

// XY to RGB conversion matrices
const double XYZToRGB[3][3] = {
    { 3.2406f, -1.5372f, -0.4986f},
    {-0.9689f,  1.8758f,  0.0415f},
    { 0.0557f, -0.2040f,  1.0570f}
};

ZigbeeLightDevice::ZigbeeLightDevice(const LightInfo& info)
    : MQTTRGBDevice(info)
{
    // Setup the RGB controller properties
    name = info.name.toStdString();
    vendor = "MQTT";  // This is critical for device filtering
    type = DEVICE_TYPE_LIGHT;
    description = "Zigbee RGB Light";
    location = "Zigbee Network";
    serial = info.unique_id.toStdString();

    // Set up modes
    modes.resize(1);
    modes[0].name = "Direct";
    modes[0].value = 0;
    modes[0].flags = MODE_FLAG_HAS_PER_LED_COLOR;
    modes[0].color_mode = MODE_COLORS_PER_LED;
    
    active_mode = 0;

    // Set up a single zone with one LED
    SetupZones();

    qDebug() << "Created ZigbeeLightDevice:" << QString::fromStdString(name)
             << "Vendor:" << QString::fromStdString(vendor)
             << "Type:" << type;
}

ZigbeeLightDevice::~ZigbeeLightDevice() = default;

void ZigbeeLightDevice::SetupZones()
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

void ZigbeeLightDevice::ResizeZone(int /*zone*/, int /*new_size*/)
{
    // Not implemented - fixed size
}

void ZigbeeLightDevice::DeviceUpdateLEDs()
{
    PublishState();
}

void ZigbeeLightDevice::UpdateZoneLEDs(int /*zone*/)
{
    PublishState();
}

void ZigbeeLightDevice::UpdateSingleLED(int /*led*/)
{
    PublishState();
}

void ZigbeeLightDevice::SetCustomMode()
{
    active_mode = 0;
}

void ZigbeeLightDevice::DeviceUpdateMode()
{
    // Not implemented - single mode only
}

RGBColor ZigbeeLightDevice::ColorTempToRGB(int temp_mireds)
{
    // Convert mireds to kelvin
    double temp_kelvin = 1000000.0 / temp_mireds;
    
    double r, g, b;
    
    // Temperature to RGB using Planckian locus approximation
    if (temp_kelvin < 6600) {
        r = 255;
        g = temp_kelvin / 100 - 2;
        g = 99.4708025861 * std::log(g) - 161.1195681661;
        if (temp_kelvin <= 1000) {
            b = 0;
        } else {
            b = temp_kelvin / 100 - 10;
            b = 138.5177312231 * std::log(b) - 305.0447927307;
        }
    } else {
        r = temp_kelvin / 100 - 55;
        r = 329.698727446 * std::pow(r, -0.1332047592);
        g = temp_kelvin / 100 - 50;
        g = 288.1221695283 * std::pow(g, -0.0755148492);
        b = 255;
    }

    // Clamp values
    r = std::max(0.0, std::min(255.0, r));
    g = std::max(0.0, std::min(255.0, g));
    b = std::max(0.0, std::min(255.0, b));

    return ToRGBColor(
        static_cast<unsigned char>(r),
        static_cast<unsigned char>(g),
        static_cast<unsigned char>(b)
    );
}

RGBColor ZigbeeLightDevice::XYToRGB(double x, double y, double brightness) 
{
    // Guard against division by zero
    if (y == 0) {
        return ToRGBColor(0, 0, 0);
    }

    // Calculate XYZ
    double z = 1.0 - x - y;
    double Y = brightness;
    double X = (Y / y) * x;
    double Z = (Y / y) * z;

    // Convert to RGB using matrix multiplication
    double r = X * XYZToRGB[0][0] + Y * XYZToRGB[0][1] + Z * XYZToRGB[0][2];
    double g = X * XYZToRGB[1][0] + Y * XYZToRGB[1][1] + Z * XYZToRGB[1][2];
    double b = X * XYZToRGB[2][0] + Y * XYZToRGB[2][1] + Z * XYZToRGB[2][2];

    // Apply gamma correction
    r = r <= 0.0031308 ? 12.92 * r : (1.0 + 0.055) * std::pow(r, (1.0 / 2.4)) - 0.055;
    g = g <= 0.0031308 ? 12.92 * g : (1.0 + 0.055) * std::pow(g, (1.0 / 2.4)) - 0.055;
    b = b <= 0.0031308 ? 12.92 * b : (1.0 + 0.055) * std::pow(b, (1.0 / 2.4)) - 0.055;

    // Convert to 0-255 range and clamp
    r = std::max(0.0, std::min(255.0, r * 255.0));
    g = std::max(0.0, std::min(255.0, g * 255.0));
    b = std::max(0.0, std::min(255.0, b * 255.0));

    return ToRGBColor(
        static_cast<unsigned char>(r),
        static_cast<unsigned char>(g),
        static_cast<unsigned char>(b)
    );
}

void ZigbeeLightDevice::UpdateFromMQTT(const QByteArray& payload)
{
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return;

    QJsonObject state = doc.object();
    qDebug() << "Zigbee light state update:" << state;
    
    // Parse power state
    QString power = state["state"].toString();
    bool is_on = (power.compare("ON", Qt::CaseInsensitive) == 0);
    
    if (!is_on) {
        colors[0] = 0x000000;
        return;
    }

    // Get brightness (0-255)
    double brightness = state["brightness"].toDouble(255) / 255.0;
    
    // Get color based on mode
    QString color_mode = state["color_mode"].toString();
    RGBColor new_color;
    
    qDebug() << "UpdateFromMQTT:" 
             << "Power:" << power 
             << "Brightness:" << brightness 
             << "Color Mode:" << color_mode;
    if (color_mode == "xy" && state.contains("color")) {
        QJsonObject color = state["color"].toObject();
        qDebug() << "Color object:" << color;
    }
    else if (color_mode == "color_temp") {
        int temp = state["color_temp"].toInt(370); // Default to mid-range
        new_color = ColorTempToRGB(temp);
        
        // Apply brightness
        unsigned char r = static_cast<unsigned char>(RGBGetRValue(new_color) * brightness);
        unsigned char g = static_cast<unsigned char>(RGBGetGValue(new_color) * brightness);
        unsigned char b = static_cast<unsigned char>(RGBGetBValue(new_color) * brightness);
        new_color = ToRGBColor(r, g, b);
    }
    else {
        // Default to white if no color information
        unsigned char value = static_cast<unsigned char>(255 * brightness);
        new_color = ToRGBColor(value, value, value);
    }

    colors[0] = new_color;
}

void ZigbeeLightDevice::PublishState()
{
    if (colors.empty()) return;

    QJsonObject state;
    state["state"] = (colors[0] == 0x000000) ? "OFF" : "ON";

    if (colors[0] != 0x000000) {
        // Get RGB values
        unsigned char r = RGBGetRValue(colors[0]);
        unsigned char g = RGBGetGValue(colors[0]);
        unsigned char b = RGBGetBValue(colors[0]);
        
        // Ensure it's always ON
        state["state"] = "ON";

        QJsonObject color;
        color["x"] = 0.42;  // Neutral x coordinate
        color["y"] = 0.365; // Neutral y coordinate
        state["color"] = color;

        qDebug() << "Fixed color publish - R:" << r << "G:" << g << "B:" << b;
    }

    QJsonDocument doc(state);
    QString publish_topic = mqtt_topic.endsWith("/set") ? mqtt_topic : mqtt_topic + "/set";
    emit mqttPublishNeeded(publish_topic, doc.toJson(QJsonDocument::Compact));
}