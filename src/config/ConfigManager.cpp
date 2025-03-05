#include "config/ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include "utils/EncryptionHelper.h"

ConfigManager::ConfigManager(QObject* parent) :
    QObject(parent)
{
    // Ensure config directory exists and set default config file
    ensureConfigDirectory();
    QString config_path = getConfigPath();
    config_file = config_path + "/openrgb2mqtt_config.json";
    
    // Check if legacy config file exists (mqtt_config.json)
    QString legacy_config_file = config_path + "/mqtt_config.json";
    bool legacy_exists = QFile(legacy_config_file).exists();
    
    // Try to load existing config
    bool config_loaded = loadConfig(config_file);
    
    // If main config not loaded but legacy exists, try to migrate settings
    if (!config_loaded && legacy_exists) {
        qDebug() << "Attempting to migrate from legacy config file";
        
        // Create default configuration first
        createDefaultConfig();
        
        // Try to load legacy config
        QFile legacy_file(legacy_config_file);
        if (legacy_file.open(QIODevice::ReadOnly)) {
            QByteArray data = legacy_file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject legacy_config = doc.object();
                
                // Copy over MQTT settings from legacy config
                if (legacy_config.contains("broker_url"))
                    config["broker_url"] = legacy_config["broker_url"];
                
                if (legacy_config.contains("broker_port"))
                    config["broker_port"] = legacy_config["broker_port"];
                
                if (legacy_config.contains("broker_username"))
                    config["broker_username"] = legacy_config["broker_username"];
                
                if (legacy_config.contains("broker_password"))
                    config["broker_password"] = legacy_config["broker_password"];
                
                if (legacy_config.contains("client_id"))
                    config["client_id"] = legacy_config["client_id"];
                
                if (legacy_config.contains("base_topic"))
                    config["base_topic"] = legacy_config["base_topic"];
                
                if (legacy_config.contains("autoconnect"))
                    config["autoconnect"] = legacy_config["autoconnect"];
                
                qDebug() << "Migrated settings from legacy config";
                
                // Save the new config with migrated settings
                if (saveConfig(config_file)) {
                    qDebug() << "Saved migrated settings to new config file";
                }
            }
            legacy_file.close();
        }
    }
    else if (!config_loaded) {
        // If no configs loaded, set default values
        createDefaultConfig();
        
        // Save default config
        saveConfig(config_file);
    }
}

ConfigManager::~ConfigManager()
{
    if (!config_file.isEmpty())
    {
        saveConfig(config_file);
    }
}

void ConfigManager::createDefaultConfig()
{
    // MQTT Settings
    config["broker_url"] = "";
    config["broker_port"] = 1883;
    config["broker_username"] = "";
    config["broker_password"] = "";
    config["client_id"] = "openrgb2mqtt";
    config["base_topic"] = "homeassistant/openrgb";
    config["autoconnect"] = false;
    
    // DDP Settings
    QJsonObject ddp;
    ddp["enabled"] = false;
    ddp["discovery_interval"] = 60; // seconds
    ddp["devices"] = QJsonArray();
    config["ddp"] = ddp;
}

QString ConfigManager::getConfigPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        .replace("/OpenRGB2MQTT", ""); // Remove the automatic app name addition
}

void ConfigManager::ensureConfigDirectory() const
{
    QDir().mkpath(getConfigPath());
}

bool ConfigManager::loadConfig(const QString& filename)
{
    config_file = filename;
    QFile file(filename);
    
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Failed to open config file for reading:" << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull())
    {
        qDebug() << "Invalid JSON in config file";
        return false;
    }

    config = doc.object();
    emit configChanged();
    return true;
}

QString ConfigManager::getBrokerUrl() const
{
    return config["broker_url"].toString();
}

void ConfigManager::setBrokerUrl(const QString& url)
{
    config["broker_url"] = url;
    saveConfig(config_file);
    emit configChanged();
    emit mqttConfigChanged();
}

QString ConfigManager::getBrokerUsername() const
{
    return config["broker_username"].toString();
}

void ConfigManager::setBrokerUsername(const QString& username)
{
    config["broker_username"] = username;
    saveConfig(config_file);
    emit configChanged();
    emit mqttConfigChanged();
}

QString ConfigManager::getBrokerPassword() const
{
    QString encrypted = config["broker_password"].toString();
    return EncryptionHelper::decryptPassword(encrypted);
}

void ConfigManager::setBrokerPassword(const QString& password)
{
    config["broker_password"] = EncryptionHelper::encryptPassword(password);
    saveConfig(config_file);
    emit configChanged();
    emit mqttConfigChanged();
}

quint16 ConfigManager::getBrokerPort() const
{
    return config["broker_port"].toInt();
}

void ConfigManager::setBrokerPort(quint16 port)
{
    config["broker_port"] = port;
    saveConfig(config_file);
    emit configChanged();
    emit mqttConfigChanged();
}

QString ConfigManager::getClientId() const
{
    return config["client_id"].toString();
}

void ConfigManager::setClientId(const QString& id)
{
    config["client_id"] = id;
    saveConfig(config_file);
    emit configChanged();
    emit mqttConfigChanged();
}

QString ConfigManager::getBaseTopic() const
{
    return config["base_topic"].toString();
}

void ConfigManager::setBaseTopic(const QString& topic)
{
    config["base_topic"] = topic;
    saveConfig(config_file);
    emit configChanged();
    emit mqttConfigChanged();
}

bool ConfigManager::getAutoConnect() const
{
    return config["autoconnect"].toBool();
}

void ConfigManager::setAutoConnect(bool enabled)
{
    config["autoconnect"] = enabled;
    saveConfig(config_file);
    emit configChanged();
    emit mqttConfigChanged();
}

// DDP Settings

bool ConfigManager::getDDPEnabled() const
{
    if (!config.contains("ddp") || !config["ddp"].isObject()) {
        return false;
    }
    
    return config["ddp"].toObject()["enabled"].toBool();
}

void ConfigManager::setDDPEnabled(bool enabled)
{
    // Ensure ddp object exists
    if (!config.contains("ddp") || !config["ddp"].isObject()) {
        config["ddp"] = QJsonObject();
    }
    
    QJsonObject ddp = config["ddp"].toObject();
    ddp["enabled"] = enabled;
    config["ddp"] = ddp;
    
    saveConfig(config_file);
    emit configChanged();
    emit ddpConfigChanged();
}

QJsonArray ConfigManager::getDDPDevices() const
{
    if (!config.contains("ddp") || !config["ddp"].isObject()) {
        return QJsonArray();
    }
    
    QJsonObject ddp = config["ddp"].toObject();
    if (!ddp.contains("devices") || !ddp["devices"].isArray()) {
        return QJsonArray();
    }
    
    return ddp["devices"].toArray();
}

void ConfigManager::setDDPDevices(const QJsonArray& devices)
{
    // Ensure ddp object exists
    if (!config.contains("ddp") || !config["ddp"].isObject()) {
        config["ddp"] = QJsonObject();
    }
    
    QJsonObject ddp = config["ddp"].toObject();
    ddp["devices"] = devices;
    config["ddp"] = ddp;
    
    saveConfig(config_file);
    emit configChanged();
    emit ddpConfigChanged();
}

int ConfigManager::getDDPDiscoveryInterval() const
{
    if (!config.contains("ddp") || !config["ddp"].isObject()) {
        return 60; // Default: 60 seconds
    }
    
    QJsonObject ddp = config["ddp"].toObject();
    return ddp["discovery_interval"].toInt(60);
}

void ConfigManager::setDDPDiscoveryInterval(int seconds)
{
    // Ensure ddp object exists
    if (!config.contains("ddp") || !config["ddp"].isObject()) {
        config["ddp"] = QJsonObject();
    }
    
    QJsonObject ddp = config["ddp"].toObject();
    ddp["discovery_interval"] = seconds;
    config["ddp"] = ddp;
    
    saveConfig(config_file);
    emit configChanged();
    emit ddpConfigChanged();
}
bool ConfigManager::isDeviceEnabled(const std::string& device_name) const
{
    QString device_key = QString::fromStdString(device_name);
    
    if (config.contains("enabled_devices") && config["enabled_devices"].isObject()) {
        QJsonObject enabled_devices = config["enabled_devices"].toObject();
        if (enabled_devices.contains(device_key)) {
            return enabled_devices[device_key].toBool();
        }
    }
    
    return false; // Default to disabled
}

void ConfigManager::setDeviceEnabled(const std::string& device_name, bool enabled)
{
    QString device_key = QString::fromStdString(device_name);
    
    QJsonObject enabled_devices;
    if (config.contains("enabled_devices") && config["enabled_devices"].isObject()) {
        enabled_devices = config["enabled_devices"].toObject();
    }
    
    enabled_devices[device_key] = enabled;
    config["enabled_devices"] = enabled_devices;
    
    // Save config immediately
    // Device state changed
    bool saved = saveConfig(config_file);
    if (!saved) {
        qWarning() << "[ConfigManager] Failed to save device state to config!";
    }
}
bool ConfigManager::forceSaveConfig()
{
    return saveConfig(config_file);
}
bool ConfigManager::saveConfig(const QString& filename)
{
    QFile file(filename);
    QFileInfo fileInfo(filename);
    
    // Make sure the directory exists
    QDir().mkpath(fileInfo.dir().path());
    
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Failed to open config file for writing:" << file.errorString();
        
        // Try again with absolute path
        QString absolutePath = QFileInfo(filename).absoluteFilePath();
        QFile absoluteFile(absolutePath);
        if (!absoluteFile.open(QIODevice::WriteOnly)) {
            // One last attempt with a hardcoded path
            QString backupPath = QDir::homePath() + "/openrgb2mqtt_config.json";
            QFile backupFile(backupPath);
            if (backupFile.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(config);
                qint64 backupBytesWritten = backupFile.write(doc.toJson());
                backupFile.flush();
                backupFile.close();
                
                // Update config_file to point to the backup location
                config_file = backupPath;
                
                return backupBytesWritten > 0;
            } else {
                return false;
            }
        } else {
            // The absolute path worked
            QJsonDocument doc(config);
            qint64 bytesWritten = absoluteFile.write(doc.toJson());
            absoluteFile.flush();
            absoluteFile.close();
            
            // Update config_file to the successful path
            config_file = absolutePath;
            
            return bytesWritten > 0;
        }
    }

    QJsonDocument doc(config);
    qint64 bytesWritten = file.write(doc.toJson());
    file.flush();
    file.close();
    
    return bytesWritten > 0;
}


