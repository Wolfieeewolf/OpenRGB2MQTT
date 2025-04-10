#pragma once

#include "OpenRGB/RGBController/RGBController.h"
#include <QString>
#include <QObject>
#include <QStringList>
#include <QByteArray>

class MQTTRGBDevice : public QObject, public RGBController
{
    Q_OBJECT

public:
    struct LightInfo {
        QString name;
        QString unique_id;
        QString state_topic;
        QString command_topic;
        QString rgb_command_template;
        QString rgb_value_template;
        int num_leds;
        bool has_brightness;
        bool has_rgb;
        bool has_effects;
        QStringList effect_list;
    };

    MQTTRGBDevice(const LightInfo& info);
    ~MQTTRGBDevice();

    // Required virtual functions
    void        SetupZones() override;
    void        ResizeZone(int zone, int new_size) override;
    void        DeviceUpdateLEDs() override;
    void        UpdateZoneLEDs(int zone) override;
    void        UpdateSingleLED(int led) override;
    void        SetCustomMode() override;
    void        DeviceUpdateMode() override;
    void        UpdateMode() override;

    // MQTT specific functions
    virtual void UpdateFromMQTT(const QByteArray& payload);
    QString     GetTopic() const { return mqtt_topic; }
    virtual void PublishState();

signals:
    // Signal for MQTT message publishing
    void mqttPublishNeeded(const QString& topic, const QByteArray& payload);

protected:
    QString mqtt_topic;
    QString rgb_command_template;
    QString rgb_value_template;
    QByteArray last_state;
    bool send_updates;
    int color_mode;

private:
};