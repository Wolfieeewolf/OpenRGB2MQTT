#pragma once

#include "MQTTRGBDevice.h"
#include <QStringList>

class ESPHomeLightDevice : public MQTTRGBDevice
{
    Q_OBJECT

public:
    ESPHomeLightDevice(const LightInfo& info);
    ~ESPHomeLightDevice();

    bool isInitialized() const { return initialized; }

    // Configuration
    void UpdateConfiguration(const LightInfo& info);

    // MQTT interface implementations
    void UpdateFromMQTT(const QByteArray& payload) override;
    void PublishState() override;

protected:
    void SetupZones() override;
    void DeviceUpdateLEDs() override;
    void UpdateZoneLEDs(int zone) override;
    void UpdateSingleLED(int led) override;
    void SetCustomMode() override;
    void DeviceUpdateMode() override;

private:
    QStringList effect_list;
    int current_effect;
    bool brightness_support;
    bool initialized;
};