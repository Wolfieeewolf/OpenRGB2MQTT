#include "ESPHomeAPIManager.h"
#include <QDebug>

ESPHomeAPIManager::ESPHomeAPIManager(QObject* parent)
    : QObject(parent)
    , dnsLookup(nullptr)
{
    qDebug() << "[ESPHome API] Initializing DNS lookup";
    dnsLookup = new QDnsLookup(QDnsLookup::ANY, "_esphomelib._tcp.local.", this);
    
    connect(dnsLookup, &QDnsLookup::finished,
            this, &ESPHomeAPIManager::handleDnsRecordLookup);
    connect(dnsLookup, &QDnsLookup::finished, 
            [this]() {
                if (dnsLookup && dnsLookup->error() != QDnsLookup::NoError) {
                    handleLookupError(dnsLookup->error());
                }
            });
    qDebug() << "[ESPHome API] Initialization complete";
}

ESPHomeAPIManager::~ESPHomeAPIManager()
{
    qDebug() << "[ESPHome API] Cleaning up";
    stopDiscovery();
    for (auto device : devices) {
        device->disconnectFromDevice();
        delete device;
    }
    devices.clear();
    if (dnsLookup) {
        delete dnsLookup;
        dnsLookup = nullptr;
    }
    qDebug() << "[ESPHome API] Cleanup complete";
}

void ESPHomeAPIManager::startDiscovery()
{
    qDebug() << "[ESPHome API] Starting discovery";
    if (!dnsLookup) {
        qDebug() << "[ESPHome API] DNS lookup not initialized";
        return;
    }

    // Try direct connection to known ESPHome device first
    QString directHost = "star-galaxy.local";
    
    QHostInfo::lookupHost(directHost,
        [this](const QHostInfo &info) {
            if (info.error() == QHostInfo::NoError && !info.addresses().isEmpty()) {
                QString address = info.addresses().first().toString();
                qDebug() << "[ESPHome API] Direct lookup found device at" << address;
                try {
                    QMetaObject::invokeMethod(this, [=]() {
                        this->addDevice("star-galaxy", address, 6053);
                    }, Qt::QueuedConnection);
                } catch (const std::exception& e) {
                    qDebug() << "[ESPHome API] Error adding device:" << e.what();
                }
            } else {
                qDebug() << "[ESPHome API] Direct lookup failed, trying mDNS";
                if (dnsLookup) {
                    dnsLookup->lookup();
                }
            }
        });
}

void ESPHomeAPIManager::stopDiscovery()
{
    if (dnsLookup) {
        dnsLookup->abort();
    }
}

std::vector<RGBController*> ESPHomeAPIManager::getDevices() const
{
    QMutexLocker locker(&device_mutex);
    std::vector<RGBController*> result;
    result.reserve(devices.size());
    for (auto device : devices) {
        if (device) {  // Null check
            result.push_back(device);
        }
    }
    return result;
}

void ESPHomeAPIManager::handleDnsRecordLookup()
{
    qDebug() << "[ESPHome API] DNS lookup finished";
    if (!dnsLookup) {
        qDebug() << "[ESPHome API] DNS lookup object is null";
        return;
    }

    if (dnsLookup->error() != QDnsLookup::NoError) {
        qDebug() << "[ESPHome API] DNS lookup error:" << dnsLookup->error() 
                 << "-" << dnsLookup->errorString();
        return;
    }

    const QList<QDnsHostAddressRecord> records = dnsLookup->hostAddressRecords();
    qDebug() << "[ESPHome API] Found" << records.size() << "address records";

    for (const QDnsHostAddressRecord &record : records) {
        QString name = record.name();
        QHostAddress address = record.value();
        
        if (name.contains("esphomelib")) {
            qDebug() << "[ESPHome API] Found device:" << name << "at" << address.toString();
            QMetaObject::invokeMethod(this, [=]() {
                this->addDevice(name, address.toString(), 6053);
            }, Qt::QueuedConnection);
        }
    }
}

void ESPHomeAPIManager::handleServiceDiscovered(const QHostInfo &info)
{
    if (info.error() != QHostInfo::NoError) {
        return;
    }

    QString name = info.hostName();
    QString address = info.addresses().first().toString();
    uint16_t port = 6053;  // Default ESPHome API port

    // Check if device supports RGB
    if (isRGBDevice(address, port)) {
        QMetaObject::invokeMethod(this, [=]() {
            this->addDevice(name, address, port);
        }, Qt::QueuedConnection);
    }
}

void ESPHomeAPIManager::handleLookupError(QDnsLookup::Error error)
{
    qDebug() << "DNS lookup error:" << error;
}

void ESPHomeAPIManager::addDevice(const QString& name, const QString& address, uint16_t port)
{
    static QMutex addDeviceMutex;
    QMutexLocker addDeviceLocker(&addDeviceMutex);

    if (name.isEmpty() || address.isEmpty() || port == 0) {
        qDebug() << "[ESPHome API] Invalid device parameters";
        return;
    }

    QMutexLocker locker(&device_mutex);
    
    // Check if device already exists
    if (devices.contains(name)) {
        qDebug() << "[ESPHome API] Device already exists:" << name;
        return;
    }

    try {
        // Prepare device info
        ESPHomeAPIDevice::DeviceInfo info;
        info.name = name;
        info.address = address;
        info.port = port;
        info.hasRGB = true;

        // Create device
        ESPHomeAPIDevice* device = new ESPHomeAPIDevice(info);
        if (!device) {
            throw std::runtime_error("Failed to create device object");
        }

        // Store device
        devices[name] = device;
        qDebug() << "[ESPHome API] Device created:" << name;

        // Connect device
        connect(device, &ESPHomeAPIDevice::connectionStatusChanged,
                this, [this, name](bool connected) {
                    qDebug() << "[ESPHome API] Device" << name << (connected ? "connected" : "disconnected");
                }, Qt::QueuedConnection);

        device->connectToDevice();

        // Notify about new device
        QMetaObject::invokeMethod(this, "deviceListChanged", Qt::QueuedConnection);
        
        qDebug() << "[ESPHome API] Successfully added device:" << name;

    } catch (const std::exception& e) {
        qDebug() << "[ESPHome API] Error adding device:" << name << "-" << e.what();
        
        // Cleanup on error
        if (devices.contains(name)) {
            if (auto* device = devices[name]) {
                device->disconnectFromDevice();
                delete device;
            }
            devices.remove(name);
        }
    }
}

void ESPHomeAPIManager::removeDevice(const QString& name)
{
    QMutexLocker locker(&device_mutex);
    auto it = devices.find(name);
    if (it != devices.end()) {
        if (*it) {  // Null check
            (*it)->disconnectFromDevice();
            delete *it;
        }
        devices.remove(name);
        emit deviceListChanged();
    }
}

bool ESPHomeAPIManager::isRGBDevice(const QString& address, uint16_t port)
{
    // We can't check capabilities until we connect, so assume it's RGB capable for now
    Q_UNUSED(address);
    Q_UNUSED(port);
    return true;
}