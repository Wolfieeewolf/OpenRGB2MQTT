#ifndef ENCRYPTIONHELPER_H
#define ENCRYPTIONHELPER_H

#include <QString>

class EncryptionHelper {
public:
    static QString encryptPassword(const QString& plaintext);
    static QString decryptPassword(const QString& encrypted);

private:
    static const QByteArray getKey();
};

#endif // ENCRYPTIONHELPER_H