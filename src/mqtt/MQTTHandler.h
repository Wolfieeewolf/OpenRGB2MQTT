#pragma once

#include <QObject>
#include <QtMqtt/qmqttclient.h>
#include <QMutex>
#include <QQueue>
#include <QPair>
#include <QTimer>

class MQTTHandler : public QObject
{
    Q_OBJECT

public:
    explicit MQTTHandler(QObject* parent = nullptr);
    ~MQTTHandler();

    void setWillMessage(const QString& topic, const QString& message);
    bool connectToHost(const QString& host, quint16 port,
                      const QString& username = QString(),
                      const QString& password = QString());
    void disconnect();
    bool isConnected() const;
    bool publish(const QString& topic, const QByteArray& payload, quint8 qos = 0, bool retain = false, bool silent = false);
    bool subscribe(const QString& topic, bool silent = false, quint8 qos = 0);

signals:
    void messageReceived(const QString& topic, const QByteArray& payload);
    void connectionStatusChanged(bool connected);
    void connectionError(const QString& error);

private slots:
    void handleMessage(const QByteArray& message, const QMqttTopicName& topic);
    void handleStateChange();
    void handleError();
    void processMessageQueue();

private:
    QMqttClient* client;
    QString willTopic;
    QString willMessage;
    QString lastError;
    mutable QMutex mutex;
    QQueue<QPair<QString, QByteArray>> messageQueue;
    QTimer* processTimer;
};