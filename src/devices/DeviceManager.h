#pragma once

#include "../../OpenRGB/RGBController/RGBController.h"
#include "../../OpenRGB/ResourceManagerInterface.h"
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <vector>
#include <string>

// Forward declarations
class MosquittoDeviceManager;
class ZigbeeDeviceManager;
class ESPHomeDeviceManager;

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    DeviceManager(ResourceManagerInterface* resource_manager, QObject* parent = nullptr);
    virtual ~DeviceManager();

    void discoverAllDevices();
    virtual void handleMQTTMessage(const QString& topic, const QByteArray& payload);
    virtual void discoverDevices();
    virtual std::vector<RGBController*> getDevices() const;

    bool setDeviceColor(const std::string& device_name, RGBColor color);
    bool setZoneColor(const std::string& device_name, const std::string& zone_name, RGBColor color);
    bool setLEDColor(const std::string& device_name, int led_index, RGBColor color);

protected:
    virtual void subscribeToTopics();
    void registerDevice(RGBController* device);
    ResourceManagerInterface* resource_manager;

signals:
    void deviceListChanged();
    void deviceColorChanged(const std::string& device_name, const RGBColor& color);
    void mqttPublishNeeded(const QString& topic, const QByteArray& payload);
    void subscriptionNeeded(const QString& topic);

public slots:
    void onProtocolDevicesChanged();

private:
    void updateDeviceList();
    RGBController* findDevice(const std::string& device_name);

    MosquittoDeviceManager* mosquitto_manager;
    ZigbeeDeviceManager* zigbee_manager;
    ESPHomeDeviceManager* esphome_manager;
    QTimer* update_timer;
    mutable QMutex device_mutex;
    std::vector<RGBController*> cached_devices;
};