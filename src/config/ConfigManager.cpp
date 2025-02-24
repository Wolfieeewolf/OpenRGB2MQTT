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
    config_file = getConfigPath() + "/mqtt_config.json";
    
    qDebug() << "Config file path:" << config_file;
    
    // Try to load existing config
    if (!loadConfig(config_file))
    {
        // Set default values
    config["broker_url"] = "";
    config["broker_port"] = 1883;
    config["broker_username"] = "";
    config["broker_password"] = "";
    config["client_id"] = "openrgb2mqtt";
    config["base_topic"] = "homeassistant/openrgb";
    config["autoconnect"] = false;
        
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
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull())
    {
        return false;
    }

    config = doc.object();
    emit configChanged();
    return true;
}

bool ConfigManager::saveConfig(const QString& filename)
{
    QFile file(filename);
    qDebug() << "Attempting to save config to:" << filename;
    QFileInfo fileInfo(filename);
    qDebug() << "Directory exists:" << fileInfo.dir().exists();
    qDebug() << "Directory is writable:" << fileInfo.dir().isReadable();
    
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Failed to open config file for writing:" << file.errorString();
        qDebug() << "File path:" << file.fileName();
        qDebug() << "Current working directory:" << QDir::currentPath();
        return false;
    }

    QJsonDocument doc(config);
    qint64 bytesWritten = file.write(doc.toJson());
    qDebug() << "Bytes written:" << bytesWritten;
    file.flush();
    file.close();
    
    // Verify the file was written
    QFileInfo writtenFile(filename);
    qDebug() << "File exists after write:" << writtenFile.exists();
    qDebug() << "File size:" << writtenFile.size();
    
    return bytesWritten > 0;
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
}