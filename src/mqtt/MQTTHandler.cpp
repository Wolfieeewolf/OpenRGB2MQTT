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
        // Connected to broker
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
        // Disconnected from broker
        emit connectionStatusChanged(false);
    }, Qt::QueuedConnection);

    // Connect to messages
    connect(client, &QMqttClient::messageReceived, this, &MQTTHandler::handleMessage);

    // Set up message processing timer - faster for RGB updates
    processTimer->setInterval(2);  // Process messages every 2ms for real-time response
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
    // Connecting to MQTT broker
    
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
        // Cannot publish when not connected
        return false;
    }

    // Silence compiler warnings for unused parameter
    (void)silent;

    // Only print publish info when needed (parameter kept for API compatibility)
    
    // Optimize QoS for different message types
    // Use QoS 0 for light commands (fastest but no guarantee) 
    // Use QoS 1 for everything else (guaranteed delivery but slower)
    quint8 effectiveQos = qos;
    bool isLightCommand = topic.contains("/set") && (topic.contains("zigbee") || topic.contains("light"));
    
    if (isLightCommand && qos == 0) {
        // For zigbee/light commands, use QoS 0 for speed
        effectiveQos = 0;
    }
    
    // For critical zigbee light commands, bypass message queue for fastest delivery
    if (isLightCommand) {
        // Prioritize - use direct mqtt client publish
        qint32 result = client->publish(QMqttTopicName(topic), payload, effectiveQos, retain);
        return result != -1;
    } else {
        // Regular publish for non-critical messages
        qint32 result = client->publish(QMqttTopicName(topic), payload, effectiveQos, retain);
        return result != -1;
    }
}

bool MQTTHandler::subscribe(const QString& topic, bool silent, quint8 qos)
{
    if (client->state() != QMqttClient::Connected) {
        // Cannot subscribe when not connected
        return false;
    }

    // Silence compiler warnings for unused parameter
    (void)silent;

    // Only print subscribe info when needed (parameter kept for API compatibility)
    auto subscription = client->subscribe(QMqttTopicFilter(topic), qos);
    if (!subscription) {
        // Failed to subscribe
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
    
    // Process up to 10 messages at once to clear backlogs quickly
    int processed = 0;
    int max_messages = 10;
    
    while (!messageQueue.isEmpty() && processed < max_messages) {
        auto msg = messageQueue.dequeue();
        processed++;
        
        // For Zigbee light commands, use direct connection for minimum latency
        // This bypasses the Qt event loop for these critical messages
        if (msg.first.contains("/set") && msg.first.contains("zigbee")) {
            // Direct call for zigbee commands to minimize latency
            emit messageReceived(msg.first, msg.second);
        } else {
            // Use queued invocation for other messages
            QMetaObject::invokeMethod(this, [=]() {
                emit messageReceived(msg.first, msg.second);
            }, Qt::QueuedConnection);
        }
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