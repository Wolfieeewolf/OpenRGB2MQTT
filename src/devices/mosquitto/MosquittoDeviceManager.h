#pragma once

#include "../DeviceManager.h"
#include "MosquittoLightDevice.h"
#include <QMap>
#include <QString>

class MosquittoDeviceManager : public QObject
{
    Q_OBJECT

public:
    MosquittoDeviceManager(QObject* parent = nullptr);
    virtual ~MosquittoDeviceManager();

    virtual void handleMQTTMessage(const QString& topic, const QByteArray& payload);
    virtual void discoverDevices();
    virtual std::vector<RGBController*> getDevices() const;

signals:
    void mqttPublishNeeded(const QString& topic, const QByteArray& payload);
    void deviceListChanged();

protected:
    virtual void subscribeToTopics();
    void processDeviceConfig(const QString& topic, const QByteArray& payload);
    void processDeviceState(const QString& topic, const QByteArray& payload);

private:
    QMap<QString, MosquittoLightDevice*> devices; // Map topic -> device
};