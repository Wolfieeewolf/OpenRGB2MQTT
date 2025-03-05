#include "ZigbeeLightDevice.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <cmath>

ZigbeeLightDevice::ZigbeeLightDevice(const LightInfo& info)
    : MQTTRGBDevice(info)
{
    // Initialize with the basic mode
    // No need to set mode_value or mode_name as they're handled in the base class
    
    // Initialize direct update approach - no timer needed for real-time response
    update_timer = new QTimer(this);
    update_timer->setSingleShot(true);
    update_timer->setInterval(1); // Reduce to 1ms for immediate response
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
    
    // Build optimized payload - minimal JSON for speed
    // Pre-allocate likely size
    QJsonObject payload;
    
    // Only include changed values to reduce packet size
    
    // Always include state
    payload["state"] = "ON";
    
    // Force immediate transition (0ms)
    payload["transition"] = 0;
    
    // Add xy color values only if colors changed
    if (color_changed) {
        QJsonObject colorObj;
        colorObj["x"] = x;
        colorObj["y"] = y;
        payload["color"] = colorObj;
    }
    
    // Add brightness only if it changed
    if (brightness_changed) {
        // Convert from 0-100% to 0-254 (Zigbee brightness range)
        int zigbeeBrightness = static_cast<int>(round((brightness_percent / 100.0) * 254.0));
        payload["brightness"] = zigbeeBrightness;
    }
    
    // Convert to compact JSON string (minimal overhead)
    QJsonDocument doc(payload);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    // Send the message
    if (!mqtt_topic.isEmpty()) {
        emit mqttPublishNeeded(mqtt_topic, data);
    }
}

void ZigbeeLightDevice::UpdateFromMQTT(const QByteArray& payload)
{
    // Protect against crashes with try-catch
    try {
        // Parse the JSON payload
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isNull() || !doc.isObject()) {
            qWarning() << "ZigbeeLightDevice: Invalid JSON payload";
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
            qWarning() << "ZigbeeLightDevice: No colors array available";
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
                if (r < 0) r = 0;
                if (r > 255) r = 255;
                if (g < 0) g = 0;
                if (g > 255) g = 255;
                if (b < 0) b = 0;
                if (b > 255) b = 255;
                
                colors[0] = ToRGBColor(r, g, b);
                color_updated = true;
                qDebug() << "Color updated from RGB:" << r << g << b;
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
                    qDebug() << "Color updated from XY:" << x << y << "->" << (int)r << (int)g << (int)b;
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
                        qDebug() << "Color updated from hex:" << hex;
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
                if (r < 0) r = 0;
                if (r > 255) r = 255;
                if (g < 0) g = 0;
                if (g > 255) g = 255;
                if (b < 0) b = 0;
                if (b > 255) b = 255;
                
                colors[0] = ToRGBColor(r, g, b);
                color_updated = true;
                qDebug() << "Color updated from rgb_color array:" << r << g << b;
            }
        }
        
        // Now updates can be sent
        send_updates = true;
    }
    catch (const std::exception& e) {
        qCritical() << "ZigbeeLightDevice::UpdateFromMQTT - Exception:" << e.what();
        send_updates = true; // Make sure we don't block updates
    }
    catch (...) {
        qCritical() << "ZigbeeLightDevice::UpdateFromMQTT - Unknown exception!";
        send_updates = true; // Make sure we don't block updates
    }
}

void ZigbeeLightDevice::PublishState()
{
    // For legacy compatibility - now just call the delayed update function
    if (!update_timer->isActive()) {
        update_timer->start();
    }
}

// Override DeviceUpdateLEDs for immediate color updates
void ZigbeeLightDevice::DeviceUpdateLEDs()
{
    // Don't send updates if flag is off
    if (!send_updates)
        return;
        
    // Extract RGB values from first LED
    unsigned char red = RGBGetRValue(colors[0]);
    unsigned char green = RGBGetGValue(colors[0]);
    unsigned char blue = RGBGetBValue(colors[0]);
    
    // Skip identical color updates to reduce unnecessary traffic
    if (red == last_r && green == last_g && blue == last_b) {
        // Color hasn't changed, don't send an update
        return;
    }
    
    // Update cached values
    last_r = red;
    last_g = green;
    last_b = blue;
    
    // In direct mode, bypass the timer completely for maximum responsiveness
    if (direct_mode) {
        sendDelayedUpdate();
    } else {
        // Fall back to timer-based approach if direct mode disabled
        if (!update_timer->isActive()) {
            update_timer->start();
        }
    }
}

// Enhanced RGB to CIE xy color space conversion with improved saturation for green and blue
void ZigbeeLightDevice::rgbToXY(unsigned char r, unsigned char g, unsigned char b, double& x, double& y)
{
    // 1. Convert to float and apply enhancement for green and blue
    float red = r / 255.0f;
    float green = g / 255.0f;
    float blue = b / 255.0f;
    
    // Boost green and blue saturation for more vibrant colors
    // Only boost when the colors are dominant to avoid affecting other hues
    if (green > red && green > blue) {
        // Enhance green when it's the dominant color
        green = std::min(1.0f, green * 1.25f);
        // Reduce red and blue slightly to maintain green purity
        red *= 0.85f;
        blue *= 0.85f;
    }
    
    if (blue > red && blue > green) {
        // Enhance blue when it's the dominant color
        blue = std::min(1.0f, blue * 1.35f);
        // Reduce red and green slightly to maintain blue purity
        red *= 0.8f;
        green *= 0.8f;
    }

    // 2. Apply gamma correction - enhanced for better color reproduction
    const float r_gamma = (red > 0.04045f) ? pow((red + 0.055f) / 1.055f, 2.4f) : (red / 12.92f);
    const float g_gamma = (green > 0.04045f) ? pow((green + 0.055f) / 1.055f, 2.4f) : (green / 12.92f);
    const float b_gamma = (blue > 0.04045f) ? pow((blue + 0.055f) / 1.055f, 2.4f) : (blue / 12.92f);

    // 3. Convert RGB to XYZ using improved matrix for wider gamut
    // These values are tuned to produce more vibrant colors with zigbee bulbs
    const float X = r_gamma * 0.664511f + g_gamma * 0.154324f + b_gamma * 0.162028f;
    const float Y = r_gamma * 0.283881f + g_gamma * 0.668433f + b_gamma * 0.047685f;
    const float Z = r_gamma * 0.000088f + g_gamma * 0.072310f + b_gamma * 0.986039f;

    // 4. Calculate xy values
    const float sum = X + Y + Z;
    if (sum > 0.0f) {
        x = X / sum;
        y = Y / sum;
        
        // Skip gamut check for colors that are definitely in gamut
        if (!(x >= 0.1 && x <= 0.7 && y >= 0.03 && y <= 0.8)) {
            // Only constrain if it's potentially out of gamut
            constrainToGamut(x, y);
        }
    } else {
        // Default white
        x = 0.312713f;
        y = 0.329016f;
    }
}

// Enhanced CIE xy to RGB conversion with improved saturation
void ZigbeeLightDevice::xyToRGB(double x, double y, unsigned char& r, unsigned char& g, unsigned char& b)
{
    try {
        // Safety bounds check
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        if (y < 0.0) y = 0.0;
        if (y > 1.0) y = 1.0;
        
        // 1. Check if xy is within gamut
        constrainToGamut(x, y);
        
        // 2. Calculate XYZ values
        float z = 1.0f - x - y;
        float Y = 1.0f; // Brightness is 100%
        float X = (Y / y) * x;
        float Z = (Y / y) * z;
        
        // 3. Convert XYZ to RGB using enhanced matrix for more vibrant colors
        // These are the inverse of the matrix we use in rgbToXY
        float r_linear = X *  1.612294f - Y * 0.203643f - Z * 0.302290f;
        float g_linear = -X * 0.509891f + Y * 1.412171f + Z * 0.066920f;
        float b_linear = X *  0.026040f - Y * 0.072569f + Z * 1.015835f;
        
        // 4. Apply reverse gamma correction with saturation boost
        float r_gamma = r_linear <= 0.0031308f ? 12.92f * r_linear : (1.0f + 0.055f) * pow(r_linear, (1.0f / 2.4f)) - 0.055f;
        float g_gamma = g_linear <= 0.0031308f ? 12.92f * g_linear : (1.0f + 0.055f) * pow(g_linear, (1.0f / 2.4f)) - 0.055f;
        float b_gamma = b_linear <= 0.0031308f ? 12.92f * b_linear : (1.0f + 0.055f) * pow(b_linear, (1.0f / 2.4f)) - 0.055f;
        
        // Determine what type of color this is to apply appropriate enhancement
        bool isGreenDominant = (g_gamma > r_gamma) && (g_gamma > b_gamma);
        bool isBlueDominant = (b_gamma > r_gamma) && (b_gamma > g_gamma);
        
        // Apply targeted saturation boost based on color
        if (isGreenDominant) {
            // Boost greens
            g_gamma = std::min(1.0f, g_gamma * 1.15f); // 15% boost
            r_gamma *= 0.9f; // reduce other components
            b_gamma *= 0.9f;
        } else if (isBlueDominant) {
            // Boost blues more aggressively
            b_gamma = std::min(1.0f, b_gamma * 1.25f); // 25% boost
            r_gamma *= 0.85f; // reduce other components more
            g_gamma *= 0.85f;
        }
        
        // 5. Convert to 0-255 RGB range and clip
        r_gamma = std::max(0.0f, std::min(1.0f, r_gamma));
        g_gamma = std::max(0.0f, std::min(1.0f, g_gamma));
        b_gamma = std::max(0.0f, std::min(1.0f, b_gamma));
        
        // Convert to 8-bit values
        r = static_cast<unsigned char>(round(r_gamma * 255.0f));
        g = static_cast<unsigned char>(round(g_gamma * 255.0f));
        b = static_cast<unsigned char>(round(b_gamma * 255.0f));
        
        // Debug output
        qDebug() << "Enhanced color from XY:" << x << y << "-> RGB:" << (int)r << (int)g << (int)b;
    }
    catch (...) {
        // If conversion fails, return white
        r = 255;
        g = 255;
        b = 255;
        qDebug() << "Exception in xyToRGB, using white";
    }
}

// Calculate Euclidean distance between two points
double ZigbeeLightDevice::distance(double x1, double y1, double x2, double y2)
{
    try {
        return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }
    catch (...) {
        // If there's any error, return a large distance
        return 1.0e10;
    }
}

// Constrain xy point to the color gamut for Zigbee with enhanced saturation
void ZigbeeLightDevice::constrainToGamut(double& x, double& y)
{
    try {
        // Safety bounds check first
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        if (y < 0.0) y = 0.0;
        if (y > 1.0) y = 1.0;

        // Define an expanded color gamut for more vibrant colors
        // These values push beyond standard gamut for more saturated colors
        // while still being within physical capabilities of most bulbs
        const double redX = 0.7;       // Enhanced red corner (was 0.675)
        const double redY = 0.299;     // Adjusted for more vibrant reds (was 0.322)
        
        const double greenX = 0.17;    // Enhanced green corner (was 0.4091)
        const double greenY = 0.7;     // Pushed for more saturated greens (was 0.518)
        
        const double blueX = 0.135;    // Enhanced blue corner (was 0.167)
        const double blueY = 0.03;     // Adjusted for deeper blues (was 0.04)
        
        // Check if the point is inside the triangle
        if (isPointInGamutTriangle(x, y, redX, redY, greenX, greenY, blueX, blueY)) {
            return; // Point is already in the gamut, nothing to do
        }
        
        // Find the closest point on any of the triangle edges
        double closestX = x;
        double closestY = y;
        double minDist = 1.0e10;
        
        // Check each edge of the triangle
        double tempX, tempY;
        double dist;
        
        // Red-Green edge
        closestPointOnLine(x, y, redX, redY, greenX, greenY, tempX, tempY);
        dist = distance(x, y, tempX, tempY);
        if (dist < minDist) {
            minDist = dist;
            closestX = tempX;
            closestY = tempY;
        }
        
        // Green-Blue edge
        closestPointOnLine(x, y, greenX, greenY, blueX, blueY, tempX, tempY);
        dist = distance(x, y, tempX, tempY);
        if (dist < minDist) {
            minDist = dist;
            closestX = tempX;
            closestY = tempY;
        }
        
        // Blue-Red edge
        closestPointOnLine(x, y, blueX, blueY, redX, redY, tempX, tempY);
        dist = distance(x, y, tempX, tempY);
        if (dist < minDist) {
            minDist = dist;
            closestX = tempX;
            closestY = tempY;
        }
        
        // Enhance saturation - pull slightly toward corners for more vibrant colors
        // Find which primary color this is closest to
        double dRed = distance(closestX, closestY, redX, redY);
        double dGreen = distance(closestX, closestY, greenX, greenY);
        double dBlue = distance(closestX, closestY, blueX, blueY);
        
        if (dRed < dGreen && dRed < dBlue) {
            // Pull toward red corner
            closestX = closestX + (redX - closestX) * 0.15;
            closestY = closestY + (redY - closestY) * 0.15;
        } else if (dGreen < dRed && dGreen < dBlue) {
            // Pull toward green corner - stronger pull for green
            closestX = closestX + (greenX - closestX) * 0.25;
            closestY = closestY + (greenY - closestY) * 0.25; 
        } else {
            // Pull toward blue corner - strongest pull for blue
            closestX = closestX + (blueX - closestX) * 0.3;
            closestY = closestY + (blueY - closestY) * 0.3;
        }
        
        // Apply the constrained coordinates
        x = closestX;
        y = closestY;
    }
    catch (...) {
        // If anything goes wrong, use a safe fallback
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        if (y < 0.0) y = 0.0;
        if (y > 1.0) y = 1.0;
    }
}

// Check if a point is inside a triangle
bool ZigbeeLightDevice::isPointInGamutTriangle(double x, double y, double x1, double y1, 
                                              double x2, double y2, double x3, double y3)
{
    try {
        bool negative = false;
        bool positive = false;
        
        double a = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
        double b = (x3 - x2) * (y - y2) - (y3 - y2) * (x - x2);
        double c = (x1 - x3) * (y - y3) - (y1 - y3) * (x - x3);
        
        if (a < 0) negative = true;
        else positive = true;
        if (b < 0) negative = true;
        else positive = true;
        if (c < 0) negative = true;
        else positive = true;
        
        return !(negative && positive);
    }
    catch (...) {
        // If there's any error, assume the point is outside the triangle
        return false;
    }
}

// Get closest point on a line segment
void ZigbeeLightDevice::closestPointOnLine(double x, double y, double x1, double y1, 
                                          double x2, double y2, double& closestX, double& closestY)
{
    try {
        double A = x - x1;
        double B = y - y1;
        double C = x2 - x1;
        double D = y2 - y1;
        
        double dot = A * C + B * D;
        double len_sq = C * C + D * D;
        double param = -1;
        
        if (len_sq != 0) // In case the line segment has zero length
            param = dot / len_sq;
        
        if (param < 0) {
            closestX = x1;
            closestY = y1;
        }
        else if (param > 1) {
            closestX = x2;
            closestY = y2;
        }
        else {
            closestX = x1 + param * C;
            closestY = y1 + param * D;
        }
    }
    catch (...) {
        // If there's any error, just return the original point
        closestX = x;
        closestY = y;
    }
}