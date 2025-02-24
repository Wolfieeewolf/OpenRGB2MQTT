#include "MQTTHandler.h"
#include <QDebug>
#include <QRandomGenerator>
#include <QMutexLocker>

MQTTHandler::MQTTHandler(QObject* parent)
    : QObject(parent)
    , client(new QMqttClient(this))
    , processTimer(new QTimer(this))
{
    // Connect state changes
    connect(client, &QMqttClient::connected, this, [this]() {
        QMutexLocker locker(&mutex);
        qInfo() << "[MQTT] Connected to broker";
        if (!willTopic.isEmpty()) {
            publish(willTopic, "online");
        }
        emit connectionStatusChanged(true);

        // Subscribe to topics silently
        subscribe("zigbee2mqtt/#", true);
        subscribe("homeassistant/light/#", true);
        subscribe("homeassistant/+/light/+/config", true);

        // Request device list
        publish("zigbee2mqtt/bridge/config/devices/get", "", 0, false, true);
    }, Qt::QueuedConnection);
    
    connect(client, &QMqttClient::disconnected, this, [this]() {
        QMutexLocker locker(&mutex);
        qInfo() << "[MQTT] Disconnected from broker";
        emit connectionStatusChanged(false);
    }, Qt::QueuedConnection);

    // Connect to messages
    connect(client, &QMqttClient::messageReceived, this, &MQTTHandler::handleMessage);

    // Set up message processing timer
    processTimer->setInterval(10);  // Process messages every 10ms
    connect(processTimer, &QTimer::timeout, this, [this]() {
        processMessageQueue();
    });
    processTimer->start();

    client->setProtocolVersion(QMqttClient::MQTT_3_1_1);
    client->setKeepAlive(60);
}

MQTTHandler::~MQTTHandler()
{
    disconnect();
    delete client;
}

void MQTTHandler::setWillMessage(const QString& topic, const QString& message)
{
    willTopic = topic;
    willMessage = message;
}

bool MQTTHandler::connectToHost(const QString& host, quint16 port,
                         const QString& username, const QString& password)
{
    qInfo() << "[MQTT] Connecting to" << host << ":" << port;
    
    if (host.isEmpty()) {
        lastError = "Invalid broker URL";
        emit connectionError(lastError);
        return false;
    }

    // Disconnect if already connected
    if (client->state() != QMqttClient::Disconnected) {
        client->disconnectFromHost();
    }

    // Basic settings
    client->setHostname(host);
    client->setPort(port);
    client->setClientId("openrgb2mqtt_" + QString::number(QRandomGenerator::global()->generate()));

    // Authentication
    if (!username.isEmpty()) {
        client->setUsername(username);
        if (!password.isEmpty()) {
            client->setPassword(password);
        }
    }

    // Will message
    if (!willTopic.isEmpty()) {
        client->setWillTopic(willTopic);
        client->setWillMessage(willMessage.toUtf8());
        client->setWillQoS(1);
        client->setWillRetain(true);
    }

    // Connection flags
    client->setCleanSession(true);

    // Try to connect
    client->connectToHost();
    
    return true;
}

void MQTTHandler::disconnect()
{
    if (client->state() != QMqttClient::Disconnected) {
        qDebug() << "MQTT: Disconnecting...";
        if (!willTopic.isEmpty()) {
            publish(willTopic, "offline");
        }
        client->disconnectFromHost();
    }
}

bool MQTTHandler::isConnected() const
{
    return client->state() == QMqttClient::Connected;
}

bool MQTTHandler::publish(const QString& topic, const QByteArray& payload, quint8 qos, bool retain, bool silent)
{
    if (client->state() != QMqttClient::Connected) {
        qWarning() << "[MQTT] Cannot publish - not connected";
        return false;
    }

    if (!silent) {
        qInfo() << "[MQTT] Publishing to:" << topic;
    }
    qint32 result = client->publish(QMqttTopicName(topic), payload, qos, retain);
    return result != -1;
}

bool MQTTHandler::subscribe(const QString& topic, bool silent, quint8 qos)
{
    if (client->state() != QMqttClient::Connected) {
        qWarning() << "[MQTT] Cannot subscribe - not connected";
        return false;
    }

    if (!silent) {
        qInfo() << "[MQTT] Subscribing to:" << topic;
    }
    auto subscription = client->subscribe(QMqttTopicFilter(topic), qos);
    if (!subscription) {
        qWarning() << "[MQTT] Failed to subscribe to:" << topic;
        return false;
    }
    return true;
}

void MQTTHandler::handleMessage(const QByteArray& message, const QMqttTopicName& topic)
{
    QMutexLocker locker(&mutex);
    messageQueue.enqueue(qMakePair(topic.name(), message));
}

void MQTTHandler::processMessageQueue()
{
    QMutexLocker locker(&mutex);
    while (!messageQueue.isEmpty()) {
        auto msg = messageQueue.dequeue();
        QMetaObject::invokeMethod(this, [=]() {
            emit messageReceived(msg.first, msg.second);
        }, Qt::QueuedConnection);
    }
}

void MQTTHandler::handleStateChange()
{
    switch(client->state()) {
        case QMqttClient::Connected:
            if (!willTopic.isEmpty()) {
                publish(willTopic, "online");
            }
            emit connectionStatusChanged(true);
            break;
            
        case QMqttClient::Disconnected:
            emit connectionStatusChanged(false);
            break;
            
        default:
            break;
    }
}

void MQTTHandler::handleError()
{
    QString errorMsg;
    auto error = client->error();
    switch (error) {
        case QMqttClient::NoError:
            return;
        case QMqttClient::InvalidProtocolVersion:
            errorMsg = "Invalid protocol version";
            break;
        case QMqttClient::IdRejected:
            errorMsg = "Client ID rejected";
            break;
        case QMqttClient::ServerUnavailable:
            errorMsg = "Server unavailable";
            break;
        case QMqttClient::BadUsernameOrPassword:
            errorMsg = "Bad username or password";
            break;
        case QMqttClient::NotAuthorized:
            errorMsg = "Not authorized";
            break;
        case QMqttClient::TransportInvalid:
            errorMsg = "Network error";
            break;
        default:
            errorMsg = QString("Unknown error: %1").arg(static_cast<int>(error));
            break;
    }
    
    qDebug() << "MQTT Error:" << errorMsg;
    lastError = errorMsg;
    emit connectionError(errorMsg);
}