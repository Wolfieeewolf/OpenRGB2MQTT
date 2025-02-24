#pragma once

#include "MQTTRGBDevice.h"

class ZigbeeLightDevice : public MQTTRGBDevice
{
    Q_OBJECT

public:
    ZigbeeLightDevice(const LightInfo& info);
    ~ZigbeeLightDevice();

    // RGBController interface implementations
    void SetupZones() override;
    void ResizeZone(int zone, int new_size) override;
    void DeviceUpdateLEDs() override;
    void UpdateZoneLEDs(int zone) override;
    void UpdateSingleLED(int led) override;
    void SetCustomMode() override;
    void DeviceUpdateMode() override;

    // MQTT interface implementations
    void UpdateFromMQTT(const QByteArray& payload) override;
    void PublishState() override;

private:
    RGBColor XYToRGB(double x, double y, double brightness);
    RGBColor ColorTempToRGB(int temp_mireds);
};