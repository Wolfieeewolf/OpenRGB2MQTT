#pragma once

#include "MQTTRGBDevice.h"
#include <QObject>
#include <QString>
#include <QTimer>

class ZigbeeLightDevice : public MQTTRGBDevice
{
    Q_OBJECT

public:
    // Constructor that matches the base class
    ZigbeeLightDevice(const LightInfo& info);
    
    // Override the base class methods
    void UpdateFromMQTT(const QByteArray& payload) override;
    void PublishState() override;
    void DeviceUpdateLEDs() override;

private slots:
    void sendDelayedUpdate();

private:
    // Color space conversion functions
    void rgbToXY(unsigned char r, unsigned char g, unsigned char b, double& x, double& y);
    void xyToRGB(double x, double y, unsigned char& r, unsigned char& g, unsigned char& b);
    
    // Utility functions for color space calculations
    void constrainToGamut(double& x, double& y);
    bool isPointInGamutTriangle(double x, double y, double x1, double y1, 
                               double x2, double y2, double x3, double y3);
    void closestPointOnLine(double x, double y, double x1, double y1, 
                           double x2, double y2, double& closestX, double& closestY);
    double distance(double x1, double y1, double x2, double y2);
    
    // A timer to debounce rapid color changes
    QTimer* update_timer = nullptr;
    
    // Cached color values to avoid sending duplicate updates
    double last_x = 0.0;
    double last_y = 0.0;
    int last_brightness = 0;
    unsigned char last_r = 0;
    unsigned char last_g = 0;
    unsigned char last_b = 0;
    bool update_pending = false;
    
    // Direct RGB update mode instead of going through update timer
    bool direct_mode = true;
};