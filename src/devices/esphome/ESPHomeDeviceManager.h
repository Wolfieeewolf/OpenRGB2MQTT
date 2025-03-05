#pragma once

#include "../DeviceManager.h"
#include "ESPHomeLightDevice.h"
#include "ESPHomeAPIManager.h"
#include <QMutex>
#include <QMap>
#include <QString>

class ESPHomeDeviceManager : public QObject
{
    Q_OBJECT

public:
    ESPHomeDeviceManager(QObject* parent = nullptr);
    ~ESPHomeDeviceManager();

    void handleMQTTMessage(const QString& topic, const QByteArray& payload);
    void initializeManager();
    void discoverDevices();
    std::vector<RGBController*> getDevices() const;

signals:
    void mqttPublishNeeded(const QString& topic, const QByteArray& payload);
    void deviceListChanged();

protected:
    void subscribeToTopics();

private:
    bool processDeviceConfig(const QString& deviceName, const QString& lightName, const QJsonObject& config);
    bool isValidRGBLight(const QJsonObject& config) const;
    void cleanupDevice(const QString& topic);

    QMap<QString, ESPHomeLightDevice*> mqtt_devices;  // Map of topic -> MQTT device
    ESPHomeAPIManager* api_manager;  // Native API device manager
    mutable QMutex device_mutex;  // Protect device access
    bool initialization_complete;  // Flag to track initialization status
};