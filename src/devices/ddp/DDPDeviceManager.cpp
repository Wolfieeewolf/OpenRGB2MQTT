#include "DDPDeviceManager.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QThread>

DDPDeviceManager::DDPDeviceManager(QObject* parent)
    : QObject(parent)
    , enabled(false)
    , discovery_timer(new QTimer(this))
{
    discovery_timer->setSingleShot(true);
    connect(discovery_timer, &QTimer::timeout, this, &DDPDeviceManager::startDiscovery);
}

DDPDeviceManager::~DDPDeviceManager()
{
    clearDevices();
    delete discovery_timer;
}

void DDPDeviceManager::setEnabled(bool enabled)
{
    if (this->enabled == enabled) return;
    
    this->enabled = enabled;
    
    if (enabled) {
        loadSavedDevices();
        // Schedule discovery for a short time later to allow UI to update
        discovery_timer->start(100);
    } else {
        discovery_timer->stop();
        clearDevices();
    }
}

bool DDPDeviceManager::isEnabled() const
{
    return enabled;
}

void DDPDeviceManager::discoverDevices()
{
    if (!enabled) return;
    
    emit discoveryStarted();
    
    // Start discovery in a small delay to allow UI to update
    discovery_timer->start(100);
}

void DDPDeviceManager::startDiscovery()
{
    if (!enabled) return;
    
    // Starting DDP device discovery
    
    // Run discovery in a separate thread to prevent UI freezing
    QThread* discovery_thread = new QThread();
    QObject* worker = new QObject();
    worker->moveToThread(discovery_thread);
    
    // Using a lambda with explicit capture instead of [this]
    connect(discovery_thread, &QThread::started, worker, [this, worker, discovery_thread]() {
        // Discover DDP devices
        QList<QPair<QString, QString>> discovered = DDPController::discoverDevices(1000);
        
        // Process discovered devices
        for (const auto& device : discovered) {
            QString ip = device.first;
            QString name = device.second;
            
            // Only add if we don't already have this device
            if (!devices.contains(ip)) {
                // Default to 60 LEDs for discovered devices
                // User can change this in the UI later
                addDevice(name, ip, 60, "Auto-discovered");
            }
        }
        
        emit discoveryFinished(discovered.size());
        
        // Clean up
        worker->deleteLater();
        discovery_thread->quit();
    });
    
    connect(discovery_thread, &QThread::finished, discovery_thread, &QThread::deleteLater);
    
    discovery_thread->start();
}

std::vector<RGBController*> DDPDeviceManager::getDevices() const
{
    std::vector<RGBController*> result;
    if (!enabled) return result;
    
    result.reserve(devices.size());
    for (auto device : devices) {
        result.push_back(device);
    }
    return result;
}

bool DDPDeviceManager::addDevice(const QString& name, const QString& ip_address, int num_leds, const QString& device_type)
{
    if (!enabled) return false;
    
    // Don't add duplicates
    if (devices.contains(ip_address)) {
        return false;
    }
    
    // Create the device
    DDPLightDevice::DeviceInfo info;
    info.name = name;
    info.ip_address = ip_address;
    info.num_leds = num_leds;
    info.device_type = device_type;
    
    DDPLightDevice* device = new DDPLightDevice(info);
    devices[ip_address] = device;
    
    // Device added to DDP manager
    
    // Save configuration
    saveSavedDevices();
    
    emit deviceListChanged();
    
    return true;
}

bool DDPDeviceManager::removeDevice(const QString& ip_address)
{
    if (!devices.contains(ip_address)) {
        return false;
    }
    
    delete devices[ip_address];
    devices.remove(ip_address);
    
    // Save configuration
    saveSavedDevices();
    
    emit deviceListChanged();
    
    return true;
}

void DDPDeviceManager::clearDevices()
{
    if (devices.isEmpty()) return;
    
    for (auto device : devices) {
        delete device;
    }
    devices.clear();
    
    emit deviceListChanged();
}

QJsonArray DDPDeviceManager::getDeviceConfig() const
{
    QJsonArray config;
    
    for (auto it = devices.constBegin(); it != devices.constEnd(); ++it) {
        DDPLightDevice* device = it.value();
        
        QJsonObject dev;
        dev["name"] = QString::fromStdString(device->name);
        dev["ip"] = device->GetIPAddress();
        dev["leds"] = static_cast<int>(device->zones[0].leds_count);
        dev["type"] = QString::fromStdString(device->description).section(" ", 0, 0); // Extract device type
        
        config.append(dev);
    }
    
    return config;
}

void DDPDeviceManager::setDeviceConfig(const QJsonArray& config)
{
    // Clear existing devices
    clearDevices();
    
    // Add devices from config
    for (const QJsonValue& value : config) {
        if (!value.isObject()) continue;
        
        QJsonObject dev = value.toObject();
        
        QString name = dev["name"].toString();
        QString ip = dev["ip"].toString();
        int leds = dev["leds"].toInt(60);
        QString type = dev["type"].toString("Generic");
        
        if (!name.isEmpty() && !ip.isEmpty()) {
            addDevice(name, ip, leds, type);
        }
    }
}

void DDPDeviceManager::loadSavedDevices()
{
    // This will be implemented with ConfigManager integration
}

void DDPDeviceManager::saveSavedDevices()
{
    // This will be implemented with ConfigManager integration
}