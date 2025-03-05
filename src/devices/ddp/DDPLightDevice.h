#pragma once

#include "../../OpenRGB/RGBController/RGBController.h"
#include "DDPController.h"
#include <QString>
#include <QObject>
#include <memory>

class DDPLightDevice : public QObject, public RGBController
{
    Q_OBJECT

public:
    struct DeviceInfo {
        QString name;
        QString ip_address;
        int num_leds;
        QString device_type;
    };

    DDPLightDevice(const DeviceInfo& info);
    ~DDPLightDevice();

    // Required RGBController methods
    void SetupZones() override;
    void ResizeZone(int zone, int new_size) override;
    void DeviceUpdateLEDs() override;
    void UpdateZoneLEDs(int zone) override;
    void UpdateSingleLED(int led) override;
    void SetCustomMode() override;
    void DeviceUpdateMode() override;
    void UpdateMode() override;
    
    // DDP specific methods
    bool Connect();
    void Disconnect();
    bool IsConnected() const;
    QString GetIPAddress() const;
    
private:
    std::unique_ptr<DDPController> controller;
    QString ip_address;
    int num_leds;
    bool is_connected;
};