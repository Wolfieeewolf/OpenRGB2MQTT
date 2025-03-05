#include "utils/EncryptionHelper.h"
#include <QCryptographicHash>

const QByteArray EncryptionHelper::getKey() {
    // Generate a key based on machine-specific data
    QByteArray baseKey = QCryptographicHash::hash(
        QSysInfo::machineUniqueId() + "OpenRGB2MQTT_KEY",
        QCryptographicHash::Sha256
    );
    return baseKey.left(32); // Use first 32 bytes for AES-256
}

QString EncryptionHelper::encryptPassword(const QString& plaintext) {
    if (plaintext.isEmpty()) {
        return QString();
    }
    
    QByteArray key = getKey();
    QByteArray data = plaintext.toUtf8();
    QByteArray encrypted;
    
    // Simple XOR encryption with the key
    for (int i = 0; i < data.size(); i++) {
        encrypted.append(data[i] ^ key[i % key.size()]);
    }
    
    return QString::fromLatin1(encrypted.toBase64());
}

QString EncryptionHelper::decryptPassword(const QString& encrypted) {
    if (encrypted.isEmpty()) {
        return QString();
    }
    
    QByteArray key = getKey();
    QByteArray data = QByteArray::fromBase64(encrypted.toLatin1());
    QByteArray decrypted;
    
    // Simple XOR decryption with the key
    for (int i = 0; i < data.size(); i++) {
        decrypted.append(data[i] ^ key[i % key.size()]);
    }
    
    return QString::fromUtf8(decrypted);
}