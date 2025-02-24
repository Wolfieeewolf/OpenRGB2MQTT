#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager();

    bool loadConfig(const QString& filename);
    bool saveConfig(const QString& filename);

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

signals:
    void configChanged();

private:
    QJsonObject config;
    QString config_file;
    
    QString getConfigPath() const;
    void ensureConfigDirectory() const;
};

#endif // CONFIGMANAGER_H