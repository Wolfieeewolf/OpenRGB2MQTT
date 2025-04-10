#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();

    bool loadConfig(const QString& filename);
    bool saveConfig(const QString& filename);
    
    // Configuration path accessor and force save
    QString getConfigPath() const;
    bool forceSaveConfig();

    // MQTT Broker settings
    QString getBrokerUrl() const;
    void setBrokerUrl(const QString& url);

    QString getBrokerUsername() const;
    void setBrokerUsername(const QString& username);

    QString getBrokerPassword() const;
    void setBrokerPassword(const QString& password);

    quint16 getBrokerPort() const;
    void setBrokerPort(quint16 port);

    QString getClientId() const;
    void setClientId(const QString& id);

    QString getBaseTopic() const;
    void setBaseTopic(const QString& topic);

    bool getAutoConnect() const;
    void setAutoConnect(bool enabled);
    
 
    // Device settings
    bool isDeviceEnabled(const std::string& device_name) const;
    void setDeviceEnabled(const std::string& device_name, bool enabled);

signals:
    void configChanged();
    void mqttConfigChanged();

public:
    // Make config file path accessible to other classes
    QString config_file;

private:
    QJsonObject config;
    
    void ensureConfigDirectory() const;
    
    // Create default configuration
    void createDefaultConfig();
};

#endif // CONFIGMANAGER_H
