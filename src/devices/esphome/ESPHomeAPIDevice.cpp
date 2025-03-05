#include "ESPHomeAPIDevice.h"
#include <QDebug>

ESPHomeAPIDevice::ESPHomeAPIDevice(const DeviceInfo& info)
    : MQTTRGBDevice(LightInfo())
    , socket(nullptr)
    , deviceAddress(info.address)
    , devicePort(info.port)
    , connected(false)
    , initialized(false)
    , deviceReady(false)
    , connectionAttempts(0)
    , reconnectTimer(nullptr)
    , handshakeCompleted(false)
    , waitingForHelloResponse(false)
    , waitingForConnectResponse(false)
    , listedEntities(false)
{
    try {
        // Initialize basic device info
        name = info.name.toStdString();
        vendor = "ESPHome";
        type = DEVICE_TYPE_LEDSTRIP;
        description = "ESPHome RGB Device";
        location = "ESPHome API";

        // Initialize modes
        modes.resize(1);
        modes[0].name = "Direct";
        modes[0].value = 0;
        modes[0].flags = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
        modes[0].color_mode = MODE_COLORS_PER_LED;

        // Initialize zones and LEDs
        SetupZones();
        active_mode = 0;

        // Create socket with proper error handling
        socket = new QTcpSocket(this);
        if (!socket) {
            throw std::runtime_error("Socket creation failed");
        }

        // Set up socket connections
        connect(socket, &QTcpSocket::connected, this, &ESPHomeAPIDevice::handleSocketConnected);
        connect(socket, &QTcpSocket::disconnected, this, &ESPHomeAPIDevice::handleSocketDisconnected);
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
                this, &ESPHomeAPIDevice::handleSocketError);
        connect(socket, &QTcpSocket::readyRead, this, &ESPHomeAPIDevice::handleSocketData);

        // Initialize reconnect timer
        reconnectTimer = new QTimer(this);
        if (!reconnectTimer) {
            throw std::runtime_error("Timer creation failed");
        }
        
        reconnectTimer->setSingleShot(true);
        connect(reconnectTimer, &QTimer::timeout, this, &ESPHomeAPIDevice::tryReconnect);

        qDebug() << "[ESPHome API] Device created:" << info.name;

    } catch (const std::exception& e) {
        qDebug() << "[ESPHome API] Creation failed:" << e.what();
        cleanup();
        throw;
    }
}

ESPHomeAPIDevice::~ESPHomeAPIDevice()
{
    QMutexLocker locker(&mutex);
    cleanup();
}

void ESPHomeAPIDevice::cleanup()
{
    // Ensure proper disconnection
    disconnectFromDevice();

    if (socket) {
        socket->disconnect();
        socket->deleteLater();
        socket = nullptr;
    }

    if (reconnectTimer) {
        reconnectTimer->stop();
        reconnectTimer->deleteLater();
        reconnectTimer = nullptr;
    }

    connected = false;
    handshakeCompleted = false;
    waitingForHelloResponse = false;
    waitingForConnectResponse = false;
    listedEntities = false;
    connectionAttempts = 0;
    receiveBuffer.clear();

    emit connectionStatusChanged(false);
}

void ESPHomeAPIDevice::connectToDevice()
{
    QMutexLocker locker(&mutex);
    
    if (!socket || connected) {
        return;
    }

    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->abort();
    }

    qDebug() << "[ESPHome API] Connecting to" << deviceAddress << ":" << devicePort;
    socket->connectToHost(deviceAddress, devicePort);
    connectionAttempts++;
}

void ESPHomeAPIDevice::disconnectFromDevice()
{
    QMutexLocker locker(&mutex);

    if (reconnectTimer && reconnectTimer->isActive()) {
        reconnectTimer->stop();
    }

    if (socket && socket->state() != QAbstractSocket::UnconnectedState) {
        socket->disconnectFromHost();
    }
}

bool ESPHomeAPIDevice::isConnected() const
{
    QMutexLocker locker(&mutex);
    return connected && handshakeCompleted;
}

void ESPHomeAPIDevice::SetupZones()
{
    zones.resize(1);
    zones[0].name = "Main";
    zones[0].type = ZONE_TYPE_SINGLE;
    zones[0].leds_min = 1;
    zones[0].leds_max = 1;
    zones[0].leds_count = 1;
    zones[0].matrix_map = NULL;

    leds.resize(1);
    leds[0].name = "Main LED";
    
    colors.resize(1);
    colors[0] = 0;
}

void ESPHomeAPIDevice::handleSocketConnected()
{
    QMutexLocker locker(&mutex);
    
    qDebug() << "[ESPHome API] Connected to" << deviceAddress;
    connected = true;
    emit connectionStatusChanged(true);
    
    connectionAttempts = 0;
    if (reconnectTimer && reconnectTimer->isActive()) {
        reconnectTimer->stop();
    }
    
    // Reset state machine
    handshakeCompleted = false;
    waitingForHelloResponse = false;
    waitingForConnectResponse = false;
    listedEntities = false;
    receiveBuffer.clear();
    
    // Start handshake
    QTimer::singleShot(100, this, [this]() {
        QMutexLocker handshakeLocker(&mutex);
        if (connected && !handshakeCompleted) {
            sendHandshake();
        }
    });
}

void ESPHomeAPIDevice::handleSocketDisconnected()
{
    QMutexLocker locker(&mutex);
    
    qDebug() << "[ESPHome API] Disconnected from" << deviceAddress;
    connected = false;
    handshakeCompleted = false;
    waitingForHelloResponse = false;
    waitingForConnectResponse = false;
    listedEntities = false;
    receiveBuffer.clear();
    
    emit connectionStatusChanged(false);

    if (!reconnectTimer->isActive() && connectionAttempts < 3) {
        QTimer::singleShot(1000, this, [this]() {
            QMutexLocker locker(&mutex);
            if (!connected && connectionAttempts < 3) {
                connectToDevice();
            }
        });
    }
}

void ESPHomeAPIDevice::handleSocketError(QAbstractSocket::SocketError error)
{
    QMutexLocker locker(&mutex);
    
    qDebug() << "[ESPHome API] Socket error:" << error << "for" << deviceAddress;
    connected = false;
    handshakeCompleted = false;
    waitingForHelloResponse = false;
    waitingForConnectResponse = false;
    listedEntities = false;
    
    emit connectionStatusChanged(false);

    socket->abort();
}

void ESPHomeAPIDevice::handleSocketData()
{
    QMutexLocker locker(&mutex);
    
    if (!socket) return;
    
    receiveBuffer.append(socket->readAll());
    processIncomingData();
}

bool ESPHomeAPIDevice::validateMessage(const QByteArray& data, uint8_t expectedType) const
{
    if (data.size() < 3) return false;
    if (static_cast<uint8_t>(data[0]) != 0x00) return false;
    if (static_cast<uint8_t>(data[2]) != expectedType) return false;
    return true;
}

void ESPHomeAPIDevice::sendMessage(uint8_t msgType, const QByteArray& data)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    try {
        QByteArray message;
        message.append(static_cast<char>(0x00));  // Protocol marker
        message.append(static_cast<char>(data.size()));  // Message size
        message.append(static_cast<char>(msgType));  // Message type
        message.append(data);  // Message data

        socket->write(message);
        socket->flush();

    } catch (const std::exception& e) {
        qDebug() << "[ESPHome API] Send error:" << e.what();
    }
}

void ESPHomeAPIDevice::processIncomingData()
{
    if (!socket || !connected || !initialized) {
        receiveBuffer.clear();
        return;
    }

    while (receiveBuffer.size() >= 3) {
    // Validate message start
    if (static_cast<uint8_t>(receiveBuffer[0]) != 0x00) {
    receiveBuffer.remove(0, 1);
    continue;
    }

    // Get message details
    uint32_t size = static_cast<uint8_t>(receiveBuffer[1]);
    uint8_t msgType = static_cast<uint8_t>(receiveBuffer[2]);

    // Validate size
    if (size > static_cast<uint32_t>(API_MAX_MESSAGE_SIZE)) {
    qDebug() << "[ESPHome API] Invalid message size:" << size;
    receiveBuffer.clear();
    return;
    }

    // Wait for complete message
    if (static_cast<uint32_t>(receiveBuffer.size()) < (size + 3)) {
    return;
    }

        // Extract message
        QByteArray data = receiveBuffer.mid(3, size);
        receiveBuffer.remove(0, size + 3);

    // Process message
    switch (msgType) {
        case API_MSG_HELLO: {
            qDebug() << "[ESPHome API] Got Hello response";
            if (waitingForHelloResponse) {
                waitingForHelloResponse = false;
                
                // Send connect message
                QByteArray connect;
                sendMessage(API_MSG_CONNECT, connect);
                waitingForConnectResponse = true;
            }
            break;
        }

            case API_MSG_CONNECT_RESPONSE: {
                qDebug() << "[ESPHome API] Got Connect response";
                if (waitingForConnectResponse) {
                    waitingForConnectResponse = false;
                    handshakeCompleted = true;
                    
                    // Request entity list
                    QByteArray list;
                    sendMessage(API_MSG_LIST_ENTITIES, list);
                }
                break;
            }

            case API_MSG_LIST_ENTITIES_DONE: {
                qDebug() << "[ESPHome API] Entity listing complete";
                if (!listedEntities) {
                    listedEntities = true;
                    
                    // Subscribe to state updates
                    QByteArray subscribe;
                    sendMessage(API_MSG_SUBSCRIBE_STATES, subscribe);
                }
                break;
            }

            case API_MSG_LIGHT_STATE:
                if (data.size() >= 5) {
                    handleLightState(data);
                }
                break;

            default:
                qDebug() << "[ESPHome API] Unknown message type:" << msgType;
                break;
        }
    }
}

void ESPHomeAPIDevice::handleLightState(const QByteArray& data)
{
    bool isOn = (data[0] != 0);
    uint8_t r = static_cast<uint8_t>(data[1]);
    uint8_t g = static_cast<uint8_t>(data[2]);
    uint8_t b = static_cast<uint8_t>(data[3]);

    colors[0] = isOn ? ToRGBColor(r, g, b) : 0;
}

void ESPHomeAPIDevice::sendHandshake()
{
    QByteArray hello;
    hello.append("OpenRGB ESPHome Client");
    hello.append('\0');  // null terminator
    hello.append(static_cast<char>(0x01));  // API version major
    hello.append(static_cast<char>(0x0A));  // API version minor (10)
    hello.append('\0');  // null terminator
    
    sendMessage(API_MSG_HELLO, hello);
    waitingForHelloResponse = true;

    // Send Connect immediately after Hello
    QTimer::singleShot(100, this, [this]() {
        QMutexLocker locker(&mutex);
        if (connected && !handshakeCompleted) {
            sendMessage(API_MSG_CONNECT);
            waitingForConnectResponse = true;
        }
    });
}

void ESPHomeAPIDevice::tryReconnect()
{
    QMutexLocker locker(&mutex);
    
    if (connectionAttempts >= 3) {
        qDebug() << "[ESPHome API] Max reconnection attempts reached for" << deviceAddress;
        if (reconnectTimer) {
            reconnectTimer->stop();
        }
        return;
    }
    
    connectToDevice();
}

void ESPHomeAPIDevice::DeviceUpdateLEDs()
{
    QMutexLocker locker(&mutex);
    
    if (!connected || !handshakeCompleted) {
        return;
    }

    QByteArray data;
    bool isOn = (colors[0] != 0);
    data.append(static_cast<char>(isOn ? 1 : 0));
    data.append(static_cast<char>(RGBGetRValue(colors[0])));
    data.append(static_cast<char>(RGBGetGValue(colors[0])));
    data.append(static_cast<char>(RGBGetBValue(colors[0])));
    data.append(static_cast<char>(0xFF));  // Full brightness

    sendMessage(API_MSG_SET_LIGHT_STATE, data);
}

void ESPHomeAPIDevice::UpdateZoneLEDs(int)
{
    DeviceUpdateLEDs();
}

void ESPHomeAPIDevice::UpdateSingleLED(int)
{
    DeviceUpdateLEDs();
}

void ESPHomeAPIDevice::SetCustomMode()
{
    active_mode = 0;
}

void ESPHomeAPIDevice::DeviceUpdateMode()
{
    // Only one mode supported
}