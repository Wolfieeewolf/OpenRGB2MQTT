#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QTimer>
#include <vector>
#include "DDPLightDevice.h"
#include "../../OpenRGB/RGBController/RGBController.h"

class DDPDeviceManager : public QObject
{
    Q_OBJECT

public:
    DDPDeviceManager(QObject* parent = nullptr);
    virtual ~DDPDeviceManager();

    // Configuration
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Device discovery and management
    void discoverDevices();
    std::vector<RGBController*> getDevices() const;
    
    // Manual device management
    bool addDevice(const QString& name, const QString& ip_address, int num_leds = 60, const QString& device_type = "Generic");
    bool removeDevice(const QString& ip_address);
    void clearDevices();
    
    // Save/load device configuration
    QJsonArray getDeviceConfig() const;
    void setDeviceConfig(const QJsonArray& config);

signals:
    void deviceListChanged();
    void discoveryStarted();
    void discoveryFinished(int count);

private slots:
    void startDiscovery();
    
private:
    bool enabled;
    QMap<QString, DDPLightDevice*> devices; // Map IP -> device
    QTimer* discovery_timer;
    
    void loadSavedDevices();
    void saveSavedDevices();
};