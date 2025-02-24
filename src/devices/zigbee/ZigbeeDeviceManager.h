#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QMutex>
#include "../DeviceManager.h"
#include "ZigbeeLightDevice.h"

class ZigbeeDeviceManager : public QObject
{
    Q_OBJECT

public:
    ZigbeeDeviceManager(QObject* parent = nullptr);
    virtual ~ZigbeeDeviceManager();

    virtual void handleMQTTMessage(const QString& topic, const QByteArray& payload);
    virtual void discoverDevices();
    virtual std::vector<RGBController*> getDevices() const;

signals:
    void mqttPublishNeeded(const QString& topic, const QByteArray& payload);
    void deviceListChanged();

protected:
    virtual void subscribeToTopics();

private:
bool isRGBLight(const QJsonObject& device) const;
QMap<QString, ZigbeeLightDevice*> devices;  // Map topic -> device
bool bridge_state_known = false;
    mutable QMutex device_mutex;

};