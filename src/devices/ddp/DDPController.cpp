#include "DDPController.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QTimer>
#include <QEventLoop>
#include <QDebug>

DDPController::DDPController(const QString& ip, QObject* parent)
    : QObject(parent)
    , socket(new QUdpSocket(this))
    , ip_address(ip)
    , device_name("Unknown DDP Device")
    , device_type("Unknown")
    , sequence(0)
    , connected(false)
{
    // Use the new-style connect syntax to avoid errors
    QObject::connect(socket, &QUdpSocket::readyRead, 
                  this, &DDPController::readPendingDatagrams);
}

DDPController::~DDPController()
{
    disconnect();
    delete socket;
}

bool DDPController::connect()
{
    if (connected) return true;
    
    if (socket->state() != QAbstractSocket::BoundState) {
        // For QUdpSocket, need to use the correct bind overload with quint16
        if (!socket->bind(0)) { // Bind to any port
            qWarning() << "Failed to bind UDP socket:" << socket->errorString();
            return false;
        }
    }
    
    // Query device status to check connection
    if (queryStatus()) {
        connected = true;
        emit connectionStatusChanged(true);
        return true;
    }
    
    return false;
}

void DDPController::disconnect()
{
    if (connected) {
        connected = false;
        emit connectionStatusChanged(false);
    }
}

bool DDPController::isConnected() const
{
    return connected;
}

bool DDPController::sendRGBData(uint32_t offset, const std::vector<RGBColor>& colors)
{
    if (colors.empty()) return true;
    
    // Maximum colors per packet to fit within typical MTU
    const int MAX_COLORS_PER_PACKET = 480;
    
    // Convert RGB colors to byte array
    QByteArray buffer;
    buffer.reserve(colors.size() * 3);
    
    for (const auto& color : colors) {
        buffer.append(static_cast<char>(RGBGetRValue(color)));
        buffer.append(static_cast<char>(RGBGetGValue(color)));
        buffer.append(static_cast<char>(RGBGetBValue(color)));
    }
    
    // Send data in chunks if needed
    for (size_t i = 0; i < colors.size(); i += MAX_COLORS_PER_PACKET) {
        size_t chunk_size = std::min(static_cast<size_t>(MAX_COLORS_PER_PACKET), 
                                   colors.size() - i);
        
        // Set push flag on last packet
        uint8_t flags = DDP_FLAGS1_VER1;
        if (i + chunk_size >= colors.size()) {
            flags |= DDP_FLAGS1_PUSH;
        }
        
        // Increment sequence number for each packet (1-15)
        sequence = (sequence % 15) + 1;
        
        // Send packet
        if (!sendPacket(flags, DDP_TYPE_RGB, DDP_ID_DISPLAY,
                      offset + (i * 3), 
                      reinterpret_cast<const uint8_t*>(buffer.constData() + (i * 3)),
                      static_cast<uint16_t>(chunk_size * 3))) {
            return false;
        }
    }
    
    return true;
}

bool DDPController::sendPushPacket()
{
    // Send empty push packet
    return sendPacket(DDP_FLAGS1_VER1 | DDP_FLAGS1_PUSH, 0, DDP_ID_DISPLAY, 0, nullptr, 0);
}

bool DDPController::queryStatus()
{
    // Send status query packet
    return sendPacket(DDP_FLAGS1_VER1 | DDP_FLAGS1_QUERY, 0, DDP_ID_STATUS, 0, nullptr, 0);
}

QList<QPair<QString, QString>> DDPController::discoverDevices(int timeout)
{
    QList<QPair<QString, QString>> discovered_devices;
    
    // Create temporary socket for discovery
    QUdpSocket discovery_socket;
    if (!discovery_socket.bind(0)) { // Bind to any port
        qWarning() << "Failed to bind discovery socket:" << discovery_socket.errorString();
        return discovered_devices;
    }
    
    // Connect to read responses
    QObject::connect(&discovery_socket, &QUdpSocket::readyRead, [&]() {
        while (discovery_socket.hasPendingDatagrams()) {
            QNetworkDatagram datagram = discovery_socket.receiveDatagram();
            QByteArray data = datagram.data();
            
            // Verify this is a DDP response (at least 10 bytes with reply flag)
            if (data.size() >= 10 && (data[0] & DDP_FLAGS1_REPLY)) {
                // Try to parse JSON status response
                QJsonDocument doc = QJsonDocument::fromJson(data.mid(10));
                if (doc.isObject()) {
                    QJsonObject status = doc.object();
                    if (status.contains("status")) {
                        QJsonObject statusObj = status["status"].toObject();
                        QString deviceName = statusObj["man"].toString() + " " + 
                                            statusObj["mod"].toString();
                        
                        // Add to discovered devices if not already present
                        QPair<QString, QString> device(datagram.senderAddress().toString(), deviceName);
                        if (!discovered_devices.contains(device)) {
                            discovered_devices.append(device);
                        }
                    }
                }
            }
        }
    });
    
    // Prepare and send broadcast discovery packet
    DDPHeader header;
    memset(&header, 0, sizeof(header));
    header.flags1 = DDP_FLAGS1_VER1 | DDP_FLAGS1_QUERY;
    header.id = DDP_ID_STATUS;
    
    // Send to broadcast address
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) && 
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            
            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            for (const QNetworkAddressEntry& entry : entries) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QHostAddress broadcast = entry.broadcast();
                    if (!broadcast.isNull()) {
                        discovery_socket.writeDatagram(
                            reinterpret_cast<const char*>(&header), 
                            sizeof(header), broadcast, DDP_PORT);
                    }
                }
            }
        }
    }
    
    // Also try the global broadcast address
    discovery_socket.writeDatagram(
        reinterpret_cast<const char*>(&header), 
        sizeof(header), QHostAddress::Broadcast, DDP_PORT);
    
    // Wait for responses
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeout);
    loop.exec();
    
    return discovered_devices;
}

void DDPController::readPendingDatagrams()
{
    while (socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = socket->receiveDatagram();
        QByteArray data = datagram.data();
        
        // Process the datagram
        if (data.size() >= 10) {
            uint8_t flags = static_cast<uint8_t>(data[0]);
            uint8_t id = static_cast<uint8_t>(data[3]);
            
            // Check if this is a reply
            if (flags & DDP_FLAGS1_REPLY) {
                switch (id) {
                    case DDP_ID_STATUS:
                        handleStatusResponse(data.mid(10)); // Skip header
                        break;
                    default:
                        // Other packet types can be handled here
                        break;
                }
            }
        }
    }
}

bool DDPController::sendPacket(uint8_t flags, uint8_t type, uint8_t id, 
                             uint32_t offset, const uint8_t* data, uint16_t length)
{
    // Initialize header
    DDPHeader header;
    memset(&header, 0, sizeof(header));
    
    header.flags1 = flags;
    header.flags2 = sequence; // Use sequence in flags2
    header.type = type;
    header.id = id;
    
    // Set offset (big-endian)
    header.offset[0] = (offset >> 24) & 0xFF;
    header.offset[1] = (offset >> 16) & 0xFF;
    header.offset[2] = (offset >> 8) & 0xFF;
    header.offset[3] = offset & 0xFF;
    
    // Set length (big-endian)
    header.len[0] = (length >> 8) & 0xFF;
    header.len[1] = length & 0xFF;
    
    // Create the full packet
    QByteArray packet(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Add data if present
    if (data != nullptr && length > 0) {
        packet.append(reinterpret_cast<const char*>(data), length);
    }
    
    // Send the packet
    qint64 bytesSent = socket->writeDatagram(packet, QHostAddress(ip_address), DDP_PORT);
    return (bytesSent == packet.size());
}

void DDPController::handleStatusResponse(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;
    
    QJsonObject root = doc.object();
    if (!root.contains("status")) return;
    
    QJsonObject status = root["status"].toObject();
    
    // Extract device information
    if (status.contains("man")) {
        QString manufacturer = status["man"].toString();
        QString model = status.contains("mod") ? status["mod"].toString() : "";
        QString version = status.contains("ver") ? status["ver"].toString() : "";
        
        device_name = manufacturer;
        if (!model.isEmpty()) device_name += " " + model;
        if (!version.isEmpty()) device_name += " (v" + version + ")";
        
        // Try to determine device type
        if (manufacturer.toLower().contains("wled")) {
            device_type = "WLED";
        } else if (manufacturer.toLower().contains("tasmota")) {
            device_type = "Tasmota";
        } else if (manufacturer.toLower().contains("esphome")) {
            device_type = "ESPHome";
        } else {
            device_type = "Generic DDP";
        }
    }
    
    // Signal the status received
    emit statusReceived(root);
    
    // Update connection status
    if (!connected) {
        connected = true;
        emit connectionStatusChanged(true);
    }
}