#pragma once

#include "../../OpenRGB/RGBController/RGBController.h"
#include "../../OpenRGB/ResourceManagerInterface.h"
#include "../config/ConfigManager.h"
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <vector>
#include <map>
#include <string>

// Forward declarations
class MosquittoDeviceManager;
class ZigbeeDeviceManager;
class ESPHomeDeviceManager;
class DDPDeviceManager;

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    DeviceManager(ResourceManagerInterface* resource_manager, QObject* parent = nullptr);
    virtual ~DeviceManager();

    /*------------------------------------------------------*\
    | Discovery and message handling                          |
    \*------------------------------------------------------*/
    void discoverAllDevices();
    virtual void handleMQTTMessage(const QString& topic, const QByteArray& payload);
    virtual void discoverDevices();
    virtual std::vector<RGBController*> getDevices() const;

    /*------------------------------------------------------*\
    | RGB control functions                                   |
    \*------------------------------------------------------*/
    bool setDeviceColor(const std::string& device_name, RGBColor color);
    bool setZoneColor(const std::string& device_name, const std::string& zone_name, RGBColor color);
    bool setLEDColor(const std::string& device_name, int led_index, RGBColor color);
    
    /*------------------------------------------------------*\
    | Protocol device managers                                |
    \*------------------------------------------------------*/
    void setDDPDeviceManager(DDPDeviceManager* manager);
    void setConfigManager(ConfigManager* manager);
    void setMQTTHandler(QObject* handler);
    /*------------------------------------------------------*\
    | Device registration control                            |
    \*------------------------------------------------------*/
    bool addDeviceToOpenRGB(const std::string& device_name, bool add);
    bool isDeviceAddedToOpenRGB(const std::string& device_name) const;
    std::vector<std::pair<std::string, std::string>> getAllAvailableDevices() const; // Returns <name, protocol>

protected:
    void registerDevice(RGBController* device);
    ResourceManagerInterface* resource_manager;

signals:
    /*------------------------------------------------------*\
    | Signals                                                |
    \*------------------------------------------------------*/
    void deviceListChanged();
    void deviceColorChanged(const std::string& device_name, const RGBColor& color);
    void mqttPublishNeeded(const QString& topic, const QByteArray& payload);
    void subscriptionNeeded(const QString& topic);

public slots:
    void onProtocolDevicesChanged();
    void onMQTTConnectionChanged(bool connected);

public:
    void updateDeviceList();
    
private:
    RGBController* findDevice(const std::string& device_name);
    void subscribeToTopics();

    MosquittoDeviceManager* mosquitto_manager;
    ZigbeeDeviceManager* zigbee_manager;
    ESPHomeDeviceManager* esphome_manager;
    DDPDeviceManager* ddp_manager;
    QTimer* update_timer;
    mutable QMutex device_mutex;
    std::vector<RGBController*> cached_devices;
    std::map<std::string, bool> devices_added_to_openrgb;  // Map of device names to their added status
    ConfigManager* config_manager;  // Reference to ConfigManager for device persistence
    QObject* mqtt_handler;      // Reference to MQTT handler for post-discovery registration
    QTimer* post_discovery_timer; // Timer for post-discovery device registration
    
    // Flag to prevent concurrent updates
    bool update_in_progress = false;
};