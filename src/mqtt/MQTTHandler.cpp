#include "MQTTHandler.h"
#include "OpenRGB/LogManager.h"
#include <QRandomGenerator>
#include <QMutexLocker>
#include <QJsonObject>
#include <QJsonDocument>

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

        // Subscribe to essential topics for auto-discovery
        subscribe("homeassistant/light/#", true);
        subscribe("homeassistant/+/light/+/config", true);
    }, Qt::QueuedConnection);
    
    connect(client, &QMqttClient::disconnected, this, [this]() {
        QMutexLocker locker(&mutex);
        // Disconnected from broker
        emit connectionStatusChanged(false);
    }, Qt::QueuedConnection);

    // Connect to messages
    connect(client, &QMqttClient::messageReceived, this, &MQTTHandler::handleMessage);

    // Set up message processing timer with appropriate interval
    processTimer->setInterval(10);  // Process messages every 10ms for efficient batching
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
    LOG_INFO("[MQTTHandler] Attempting to connect to MQTT broker: %s:%d", qUtf8Printable(host), port);
    
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
    
    // Use a simpler client ID
    client->setClientId("openrgb2mqtt");

    // Authentication
    if (!username.isEmpty()) {
        client->setUsername(username);
        LOG_INFO("[MQTTHandler] Using username: %s", qUtf8Printable(username));
        if (!password.isEmpty()) {
            client->setPassword(password);
            LOG_INFO("[MQTTHandler] Password provided");
        }
    }

    // Will message (disable for testing)
    //if (!willTopic.isEmpty()) {
    //    client->setWillTopic(willTopic);
    //    client->setWillMessage(willMessage.toUtf8());
    //    client->setWillQoS(1);
    //    client->setWillRetain(true);
    //}

    // Connection flags
    client->setCleanSession(true);

    // Try to connect
    LOG_INFO("[MQTTHandler] Connecting to MQTT broker...");
    client->connectToHost();
    
    // Test the connection after a delay
    QTimer::singleShot(5000, this, [this, host, port, username, password]() {
        if (client->state() == QMqttClient::Connected) {
            LOG_INFO("[MQTTHandler] Connection successful to %s:%d.", 
                     qUtf8Printable(host), port);
        } else {
            LOG_ERROR("[MQTTHandler] Not connected to %s:%d after 5 seconds", 
                     qUtf8Printable(host), port);
            
            // Try Home Assistant default credentials as a fallback
            LOG_INFO("[MQTTHandler] Trying fallback connection to core-mosquitto:1883");
            client->disconnectFromHost();
            client->setHostname("core-mosquitto");
            client->setPort(1883);
            client->setUsername("homeassistant");
            client->setPassword("homeassistant");
            client->connectToHost();
            
            QTimer::singleShot(3000, this, [this]() {
                if (client->state() == QMqttClient::Connected) {
                    LOG_INFO("[MQTTHandler] Fallback connection successful.");
                } else {
                    LOG_ERROR("[MQTTHandler] Fallback connection failed");
                }
            });
        }
    });
    
    return true;
}

void MQTTHandler::disconnect()
{
    if (client->state() != QMqttClient::Disconnected) {
        LOG_INFO("MQTT: Disconnecting...");
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
        LOG_WARNING("[MQTTHandler] Cannot publish when not connected. Topic: %s", qUtf8Printable(topic));
        return false;
    }

    // Silence compiler warnings for unused parameter
    (void)silent;

    // Standard MQTT publish with requested QoS level
    LOG_INFO("[MQTTHandler] Publishing to topic: %s, payload: %s", qUtf8Printable(topic), payload.constData());
    qint32 result = client->publish(QMqttTopicName(topic), payload, qos, retain);
    
    if (result == -1) {
        LOG_ERROR("[MQTTHandler] Failed to publish to topic: %s", qUtf8Printable(topic));
        return false;
    }
    
    LOG_INFO("[MQTTHandler] Successfully published to topic: %s", qUtf8Printable(topic));
    return true;
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
    // Log every message received
    LOG_INFO("[MQTTHandler] Received message on topic: %s", qUtf8Printable(topic.name()));
    
    QMutexLocker locker(&mutex);
    messageQueue.enqueue(qMakePair(topic.name(), message));
}

void MQTTHandler::processMessageQueue()
{
    QMutexLocker locker(&mutex);
    
    // Process up to 20 messages at once to clear backlogs efficiently
    int processed = 0;
    int max_messages = 20;
    
    while (!messageQueue.isEmpty() && processed < max_messages) {
        auto msg = messageQueue.dequeue();
        processed++;
        
        // Emit the message - unified approach for all message types
        emit messageReceived(msg.first, msg.second);
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
    
    LOG_ERROR("MQTT Error: %s", qUtf8Printable(errorMsg));
    lastError = errorMsg;
    emit connectionError(errorMsg);
}