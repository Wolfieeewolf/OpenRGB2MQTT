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

signals:
    // Both signal types for compatibility
    void publishMessage(const QString& topic, const QByteArray& payload);
    void mqttPublishNeeded(const QString& topic, const QByteArray& payload);

private slots:
    void sendDelayedUpdate();

private:
    // Color space conversion functions
    void rgbToXY(unsigned char r, unsigned char g, unsigned char b, double& x, double& y);
    void xyToRGB(double x, double y, unsigned char& r, unsigned char& g, unsigned char& b);
    
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
};