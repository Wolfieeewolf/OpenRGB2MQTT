#pragma once

#include <QMutex>
#include <QMap>
#include "ESPHomeAPIDevice.h"
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QDnsLookup>

class ESPHomeAPIManager : public QObject
{
    Q_OBJECT

public:
    explicit ESPHomeAPIManager(QObject* parent = nullptr);
    ~ESPHomeAPIManager();

    void startDiscovery();
    void stopDiscovery();
    std::vector<RGBController*> getDevices() const;

signals:
    void deviceListChanged();

private slots:
    void handleServiceDiscovered(const QHostInfo &info);
    void handleLookupError(QDnsLookup::Error error);
    void handleDnsRecordLookup();

private:
    void addDevice(const QString& name, const QString& address, uint16_t port);
    void removeDevice(const QString& name);
    bool isRGBDevice(const QString& address, uint16_t port);

    QDnsLookup* dnsLookup;
    QMap<QString, ESPHomeAPIDevice*> devices;  // Map of device name -> device
    mutable QMutex device_mutex;  // Protect device access
};