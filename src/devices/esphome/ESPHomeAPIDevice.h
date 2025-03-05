#pragma once

#include "MQTTRGBDevice.h"
#include <QTimer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QEventLoop>
#include <QMutex>

// ESPHome API message types
#define API_MSG_HELLO                  0x00
#define API_MSG_CONNECT               0x01
#define API_MSG_CONNECT_RESPONSE      0x02
#define API_MSG_LIST_ENTITIES         0x0B
#define API_MSG_LIST_ENTITIES_DONE    0x13
#define API_MSG_SUBSCRIBE_STATES      0x14
#define API_MSG_LIGHT_STATE           0x19
#define API_MSG_SET_LIGHT_STATE       0x20

// Maximum message size to prevent buffer overflows
#define API_MAX_MESSAGE_SIZE          1024

class ESPHomeAPIDevice : public MQTTRGBDevice
{
    Q_OBJECT

public:
    // Add initialization status check
    bool isInitialized() const { QMutexLocker locker(&mutex); return initialized && deviceReady; }
    struct DeviceInfo {
        QString name;
        QString address;
        uint16_t port;
        bool hasRGB;
        QStringList effects;
    };

    ESPHomeAPIDevice(const DeviceInfo& info);
    ~ESPHomeAPIDevice();

    void connectToDevice();
    void disconnectFromDevice();
    bool isConnected() const;

signals:
    void connectionStatusChanged(bool connected);

protected:
    void SetupZones() override;
    void DeviceUpdateLEDs() override;
    void UpdateZoneLEDs(int zone) override;
    void UpdateSingleLED(int led) override;
    void SetCustomMode() override;
    void DeviceUpdateMode() override;

private slots:
    void handleSocketConnected();
    void handleSocketDisconnected();
    void handleSocketError(QAbstractSocket::SocketError error);
    void handleSocketData();

private:
    void sendHandshake();
    void sendMessage(uint8_t msgType, const QByteArray& data = QByteArray());
    void processIncomingData();
    void handleLightState(const QByteArray& data);
    void tryReconnect();
    void cleanup();
    bool validateMessage(const QByteArray& data, uint8_t expectedType) const;

    mutable QMutex mutex;
    QTcpSocket* socket;
    QString deviceAddress;
    uint16_t devicePort;
    bool connected;
    int connectionAttempts;
    QTimer* reconnectTimer;
    
    // Handshake state
    bool handshakeCompleted;
    bool waitingForHelloResponse;
    bool waitingForConnectResponse;
    bool listedEntities;
    bool initialized;
    bool deviceReady;
    QByteArray receiveBuffer;
};