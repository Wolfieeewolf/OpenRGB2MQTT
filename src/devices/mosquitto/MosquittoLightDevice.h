#pragma once

#include "MQTTRGBDevice.h"

class MosquittoLightDevice : public MQTTRGBDevice
{
    Q_OBJECT

public:
    MosquittoLightDevice(const LightInfo& info);
    ~MosquittoLightDevice();

    void UpdateFromMQTT(const QByteArray& payload) override;
    void PublishState() override;

private:
    QString formatStatePayload() const;
    void parseStatePayload(const QByteArray& payload);
};