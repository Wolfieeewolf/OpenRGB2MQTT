#pragma once

#include <QString>
#include <QUdpSocket>
#include <QHostAddress>
#include <QList>
#include <QPair>
#include <QByteArray>
#include <vector>
#include "../../OpenRGB/RGBController/RGBController.h"

// DDP Protocol Definitions
#define DDP_PORT 4048

// DDP Flags
#define DDP_FLAGS1_VER     0xc0
#define DDP_FLAGS1_VER1    0x40
#define DDP_FLAGS1_PUSH    0x01
#define DDP_FLAGS1_QUERY   0x02
#define DDP_FLAGS1_REPLY   0x04
#define DDP_FLAGS1_STORAGE 0x08
#define DDP_FLAGS1_TIME    0x10

// DDP IDs
#define DDP_ID_DISPLAY     1
#define DDP_ID_CONFIG      250
#define DDP_ID_STATUS      251

// DDP Data Types
#define DDP_TYPE_RGB       0x01
#define DDP_TYPE_RGBW      0x03

// DDP header structure
struct DDPHeader {
    uint8_t flags1;
    uint8_t flags2;
    uint8_t type;
    uint8_t id;
    uint8_t offset[4];  // MSB first
    uint8_t len[2];     // MSB first
};

class DDPController : public QObject
{
    Q_OBJECT

public:
    DDPController(const QString& ip, QObject* parent = nullptr);
    ~DDPController();
    
    // Core DDP functions
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    // Send RGB data to device
    bool sendRGBData(uint32_t offset, const std::vector<RGBColor>& colors);
    
    // Send push packet for synchronization
    bool sendPushPacket();
    
    // Query device information
    bool queryStatus();
    
    // Static discovery function
    static QList<QPair<QString, QString>> discoverDevices(int timeout = 500);
    
    // Getters
    QString getIPAddress() const { return ip_address; }
    QString getDeviceName() const { return device_name; }
    QString getDeviceType() const { return device_type; }
    
signals:
    void connectionStatusChanged(bool connected);
    void statusReceived(const QJsonObject& status);
    
private slots:
    void readPendingDatagrams();
    
private:
    QUdpSocket* socket;
    QString ip_address;
    QString device_name;
    QString device_type;
    uint8_t sequence;
    bool connected;
    
    // Helper for creating and sending DDP packets
    bool sendPacket(uint8_t flags, uint8_t type, uint8_t id, 
                   uint32_t offset, const uint8_t* data, uint16_t length);
                   
    // Parse status response
    void handleStatusResponse(const QByteArray& data);
};