#include "ZigbeeLightDevice.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "OpenRGB/LogManager.h"
#include <cmath>

ZigbeeLightDevice::ZigbeeLightDevice(const LightInfo& info)
    : MQTTRGBDevice(info)
{
    // Initialize with the basic mode
    // No need to set mode_value or mode_name as they're handled in the base class
    
    // Log the MQTT topic to verify it's correct
    LOG_INFO("[ZigbeeLightDevice] Created device with name: %s", info.name.toStdString().c_str());
    LOG_INFO("[ZigbeeLightDevice] MQTT topic: %s", mqtt_topic.toStdString().c_str());
    
    // Check if the topic format is correct for zigbee2mqtt
    if (!mqtt_topic.contains("/set")) {
        LOG_WARNING("[ZigbeeLightDevice] MQTT topic may be incorrect - missing /set suffix: %s", 
                  mqtt_topic.toStdString().c_str());
    }
    
    // Initialize direct update approach - use timer for batching updates
    update_timer = new QTimer(this);
    update_timer->setSingleShot(true);
    update_timer->setInterval(5); // 5ms interval for batching
    connect(update_timer, &QTimer::timeout, this, &ZigbeeLightDevice::sendDelayedUpdate);
    
    // Initialize color caching variables
    last_r = 0;
    last_g = 0;
    last_b = 0;
    last_brightness = 0;
}

// Highly optimized color update implementation for real-time response
void ZigbeeLightDevice::sendDelayedUpdate()
{
    if (!send_updates || colors.size() == 0) {
        return;
    }

    // Extract RGB values from first LED
    unsigned char red = RGBGetRValue(colors[0]);
    unsigned char green = RGBGetGValue(colors[0]);
    unsigned char blue = RGBGetBValue(colors[0]);
    
    // Extract brightness if supported by the active mode
    int brightness_percent = 100;
    bool brightness_changed = false;
    
    if (active_mode < modes.size() && (modes[active_mode].flags & MODE_FLAG_HAS_BRIGHTNESS)) {
        brightness_percent = modes[active_mode].brightness;
        if (brightness_percent < 0) brightness_percent = 0;
        if (brightness_percent > 100) brightness_percent = 100;
        
        // Check if brightness changed
        if (brightness_percent != last_brightness) {
            last_brightness = brightness_percent;
            brightness_changed = true;
        }
    }
    
    // Convert RGB to xy color space for Zigbee compatibility - only if color changed
    double x, y;
    rgbToXY(red, green, blue, x, y);
    
    // Skip if the color values haven't changed (within small tolerance)
    const double tolerance = 0.0001;
    bool color_changed = (std::abs(x - last_x) > tolerance || std::abs(y - last_y) > tolerance);
    
    if (!color_changed && !brightness_changed) {
        // Neither color nor brightness changed, skip update
        return;
    }
    
    // Update cached values
    last_x = x;
    last_y = y;
    
    // Create JSON string directly
    QString jsonStr;
    
    // Include brightness if changed
    if (brightness_changed) {
        // Convert from 0-100% to 0-254 (Zigbee brightness range)
        int zigbeeBrightness = static_cast<int>(round((brightness_percent / 100.0) * 254.0));
        
        if (color_changed) {
            // Both color and brightness changed
            jsonStr = QString("{\"state\":\"ON\",\"color\":{\"x\":%1,\"y\":%2},\"brightness\":%3}")
                     .arg(x).arg(y).arg(zigbeeBrightness);
        } else {
            // Only brightness changed
            jsonStr = QString("{\"state\":\"ON\",\"brightness\":%1}")
                     .arg(zigbeeBrightness);
        }
    } else if (color_changed) {
        // Only color changed
        jsonStr = QString("{\"state\":\"ON\",\"color\":{\"x\":%1,\"y\":%2}}")
                 .arg(x).arg(y);
    } else {
        // Nothing changed - can't reach here due to earlier check but include for safety
        jsonStr = "{\"state\":\"ON\"}";
    }
    
    QByteArray data = jsonStr.toUtf8();
    
    // Send the message directly to zigbee2mqtt topic
    QString deviceName = QString::fromStdString(name);
    QString setTopic = QString("zigbee2mqtt/%1/set").arg(deviceName);
    
    LOG_INFO("[ZigbeeLightDevice] Sending color [R:%d G:%d B:%d] to topic: %s", 
             red, green, blue, qUtf8Printable(setTopic));
    LOG_INFO("[ZigbeeLightDevice] Payload: %s", data.constData());
    
    // Try both signal types to ensure delivery
    emit publishMessage(setTopic, data);
    emit mqttPublishNeeded(setTopic, data);
}

void ZigbeeLightDevice::UpdateFromMQTT(const QByteArray& payload)
{
    // Protect against crashes with try-catch
    try {
        // Parse the JSON payload
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isNull() || !doc.isObject()) {
            LOG_WARNING("ZigbeeLightDevice: Invalid JSON payload");
            return;
        }

        // Don't send updates during processing
        send_updates = false;

        QJsonObject obj = doc.object();
        
        // Extract state
        bool is_on = true;
        if (obj.contains("state")) {
            QString state = obj["state"].toString();
            is_on = (state.toUpper() == "ON");
        }
        
        // If light is off, set all LEDs to black
        if (!is_on) {
            for(unsigned int i = 0; i < colors.size(); i++) {
                colors[i] = 0x000000;
            }
            send_updates = true;
            return;
        }
        
        // Extract brightness if available
        if (obj.contains("brightness")) {
            int brightness = obj["brightness"].toInt();
            // Zigbee brightness is 0-254, convert to 0-100%
            double percent = (brightness / 254.0) * 100.0;
            int brightness_percent = static_cast<int>(round(percent));
            
            // Ensure within range
            if (brightness_percent < 0) brightness_percent = 0;
            if (brightness_percent > 100) brightness_percent = 100;
            
            // Set brightness in all modes that support it
            for (auto& mode : modes) {
                if (mode.flags & MODE_FLAG_HAS_BRIGHTNESS) {
                    mode.brightness = brightness_percent;
                }
            }
        }
        
        // Now start with color handling - use safeguards
        if (colors.size() <= 0) {
            LOG_WARNING("ZigbeeLightDevice: No colors array available");
            send_updates = true;
            return;
        }
        
        bool color_updated = false;
        
        // Extract color data - try different formats
        if (obj.contains("color")) {
            QJsonObject colorObj = obj["color"].toObject();
            
            // Method 1: If we have RGB values directly
            if (colorObj.contains("r") && colorObj.contains("g") && colorObj.contains("b")) {
                int r = colorObj["r"].toInt();
                int g = colorObj["g"].toInt();
                int b = colorObj["b"].toInt();
                
                // Clamp values to valid range
                r = qBound(0, r, 255);
                g = qBound(0, g, 255);
                b = qBound(0, b, 255);
                
                colors[0] = ToRGBColor(r, g, b);
                color_updated = true;
                LOG_DEBUG("Color updated from RGB: %d %d %d", r, g, b);
            }
            // Method 2: If we have xy color
            else if (colorObj.contains("x") && colorObj.contains("y")) {
                double x = colorObj["x"].toDouble();
                double y = colorObj["y"].toDouble();
                
                // Validate xy values to prevent crashes
                if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
                    // Convert xy to RGB
                    unsigned char r, g, b;
                    xyToRGB(x, y, r, g, b);
                    
                    colors[0] = ToRGBColor(r, g, b);
                    color_updated = true;
                    LOG_DEBUG("Color updated from XY: %f %f -> %d %d %d", x, y, (int)r, (int)g, (int)b);
                }
            }
            // Method 3: Hex format
            else if (colorObj.contains("hex")) {
                QString hex = colorObj["hex"].toString();
                if (hex.startsWith("#") && hex.length() >= 7) {
                    bool ok;
                    int r = hex.mid(1, 2).toInt(&ok, 16);
                    int g = hex.mid(3, 2).toInt(&ok, 16);
                    int b = hex.mid(5, 2).toInt(&ok, 16);
                    
                    if (ok) {
                        colors[0] = ToRGBColor(r, g, b);
                        color_updated = true;
                        LOG_DEBUG("Color updated from hex: %s", qUtf8Printable(hex));
                    }
                }
            }
        }
        
        // Alternative method 4 - RGB array
        if (!color_updated && obj.contains("rgb_color") && obj["rgb_color"].isArray()) {
            QJsonArray rgbArray = obj["rgb_color"].toArray();
            if (rgbArray.size() >= 3) {
                int r = rgbArray[0].toInt();
                int g = rgbArray[1].toInt();
                int b = rgbArray[2].toInt();
                
                // Clamp values to valid range
                r = qBound(0, r, 255);
                g = qBound(0, g, 255);
                b = qBound(0, b, 255);
                
                colors[0] = ToRGBColor(r, g, b);
                color_updated = true;
                LOG_DEBUG("Color updated from rgb_color array: %d %d %d", r, g, b);
            }
        }
        
        // Now updates can be sent
        send_updates = true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("ZigbeeLightDevice::UpdateFromMQTT - Exception: %s", e.what());
        send_updates = true; // Make sure we don't block updates
    }
    catch (...) {
        LOG_ERROR("ZigbeeLightDevice::UpdateFromMQTT - Unknown exception!");
        send_updates = true; // Make sure we don't block updates
    }
}

void ZigbeeLightDevice::PublishState()
{
    LOG_INFO("[ZigbeeLightDevice] PublishState called");
    
    // Don't use timer here - send update immediately
    if (colors.size() > 0) {
        // Extract RGB values from first LED
        unsigned char red = RGBGetRValue(colors[0]);
        unsigned char green = RGBGetGValue(colors[0]);
        unsigned char blue = RGBGetBValue(colors[0]);
        
        // Convert RGB to xy color space
        double x, y;
        rgbToXY(red, green, blue, x, y);
        
        // Build the JSON payload directly as a string
        QString jsonStr;
        
        // Check if we need to include brightness
        if (active_mode < modes.size() && (modes[active_mode].flags & MODE_FLAG_HAS_BRIGHTNESS)) {
            int brightness_percent = modes[active_mode].brightness;
            if (brightness_percent < 0) brightness_percent = 0;
            if (brightness_percent > 100) brightness_percent = 100;
            
            // Convert from 0-100% to 0-254 (Zigbee brightness range)
            int zigbeeBrightness = static_cast<int>(round((brightness_percent / 100.0) * 254.0));
            
            // Include brightness in the payload
            jsonStr = QString("{\"state\":\"ON\",\"color\":{\"x\":%1,\"y\":%2},\"brightness\":%3}")
                     .arg(x).arg(y).arg(zigbeeBrightness);
        } else {
            // Just color, no brightness
            jsonStr = QString("{\"state\":\"ON\",\"color\":{\"x\":%1,\"y\":%2}}")
                     .arg(x).arg(y);
        }
        
        QByteArray data = jsonStr.toUtf8();
        
        // Send directly to the zigbee topic
        QString deviceName = QString::fromStdString(name);
        QString setTopic = QString("zigbee2mqtt/%1/set").arg(deviceName);
        
        LOG_INFO("[ZigbeeLightDevice] Publishing state to %s with payload: %s", 
                 qUtf8Printable(setTopic), data.constData());
        
        // Send both signals
        emit publishMessage(setTopic, data);
        emit mqttPublishNeeded(setTopic, data);
    }
}

// Override DeviceUpdateLEDs for immediate color updates
void ZigbeeLightDevice::DeviceUpdateLEDs()
{
    // Don't send updates if flag is off
    if (!send_updates || colors.size() == 0)
        return;
        
    // Extract RGB values from first LED
    unsigned char red = RGBGetRValue(colors[0]);
    unsigned char green = RGBGetGValue(colors[0]);
    unsigned char blue = RGBGetBValue(colors[0]);
    
    // Skip identical color updates to reduce unnecessary traffic
    if (red == last_r && green == last_g && blue == last_b) {
        return;
    }
    
    // Update cached values
    last_r = red;
    last_g = green;
    last_b = blue;
    
    // Convert RGB to xy color space
    double x, y;
    rgbToXY(red, green, blue, x, y);
    
    // Create simple JSON string
    QString jsonStr = QString("{\"state\":\"ON\",\"color\":{\"x\":%1,\"y\":%2}}").arg(x).arg(y);
    QByteArray data = jsonStr.toUtf8();
    
    // Send directly to the zigbee topic
    QString deviceName = QString::fromStdString(name);
    QString setTopic = QString("zigbee2mqtt/%1/set").arg(deviceName);
    
    // Send directly to MQTT
    emit mqttPublishNeeded(setTopic, data);
}

// Streamlined RGB to CIE xy color space conversion
void ZigbeeLightDevice::rgbToXY(unsigned char r, unsigned char g, unsigned char b, double& x, double& y)
{
    // 1. Convert to float
    float red = r / 255.0f;
    float green = g / 255.0f;
    float blue = b / 255.0f;
    
    // 2. Apply gamma correction - simplified for speed
    const float r_gamma = (red > 0.04045f) ? pow((red + 0.055f) / 1.055f, 2.4f) : (red / 12.92f);
    const float g_gamma = (green > 0.04045f) ? pow((green + 0.055f) / 1.055f, 2.4f) : (green / 12.92f);
    const float b_gamma = (blue > 0.04045f) ? pow((blue + 0.055f) / 1.055f, 2.4f) : (blue / 12.92f);

    // 3. Convert RGB to XYZ using matrix for standard conversion
    const float X = r_gamma * 0.649926f + g_gamma * 0.103455f + b_gamma * 0.197109f;
    const float Y = r_gamma * 0.234327f + g_gamma * 0.743075f + b_gamma * 0.022598f;
    const float Z = r_gamma * 0.000000f + g_gamma * 0.053077f + b_gamma * 1.035763f;

    // 4. Calculate xy values
    const float sum = X + Y + Z;
    if (sum > 0.0f) {
        x = X / sum;
        y = Y / sum;
    } else {
        // Default white
        x = 0.312713f;
        y = 0.329016f;
    }
}

// Simplified CIE xy to RGB conversion
void ZigbeeLightDevice::xyToRGB(double x, double y, unsigned char& r, unsigned char& g, unsigned char& b)
{
    // Safety bounds check
    if (x < 0.0) x = 0.0;
    if (x > 1.0) x = 1.0;
    if (y < 0.0) y = 0.0;
    if (y > 1.0) y = 1.0;
    
    // Calculate XYZ
    float Y = 1.0f; // Brightness is handled separately
    float X = (Y / y) * x;
    float Z = (Y / y) * (1.0f - x - y);
    
    // Convert to RGB
    float r_linear = X * 1.656492f - Y * 0.354851f - Z * 0.255038f;
    float g_linear = -X * 0.707196f + Y * 1.655397f + Z * 0.036152f;
    float b_linear = X * 0.051713f - Y * 0.121364f + Z * 1.011530f;
    
    // Apply reverse gamma correction
    float r_gamma = (r_linear <= 0.0031308f) ? 12.92f * r_linear : 1.055f * pow(r_linear, 1/2.4f) - 0.055f;
    float g_gamma = (g_linear <= 0.0031308f) ? 12.92f * g_linear : 1.055f * pow(g_linear, 1/2.4f) - 0.055f;
    float b_gamma = (b_linear <= 0.0031308f) ? 12.92f * b_linear : 1.055f * pow(b_linear, 1/2.4f) - 0.055f;
    
    // Clip values
    r_gamma = std::max(0.0f, std::min(1.0f, r_gamma));
    g_gamma = std::max(0.0f, std::min(1.0f, g_gamma));
    b_gamma = std::max(0.0f, std::min(1.0f, b_gamma));
    
    // Convert to 8-bit
    r = static_cast<unsigned char>(round(r_gamma * 255.0f));
    g = static_cast<unsigned char>(round(g_gamma * 255.0f));
    b = static_cast<unsigned char>(round(b_gamma * 255.0f));
}