#pragma once

#include <QObject>
#include <QMutex>
#include <QMenu>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include "OpenRGB/OpenRGBPluginInterface.h"
#include "OpenRGB/ResourceManagerInterface.h"

class OpenRGB2MQTT : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID)
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    OpenRGB2MQTT();
    ~OpenRGB2MQTT();

    virtual OpenRGBPluginInfo GetPluginInfo() override;
    virtual unsigned int GetPluginAPIVersion() override;
    virtual void Load(ResourceManagerInterface* resource_manager_ptr) override;
void delayedInit();
    virtual QWidget* GetWidget() override;
    virtual QMenu* GetTrayMenu() override;
    virtual void Unload() override;

private:
    void createMainWidget();
    void initializeConnection();
    void cleanup();
    
    bool dark_theme;
    ResourceManagerInterface* resource_manager;
    class MQTTHandler* mqtt_handler;
    class DeviceManager* device_manager;
    class ConfigManager* config_manager;
    QWidget* main_widget;
    QMutex init_mutex;
    bool components_initialized;
    
    // UI elements
    QLabel* status_label;
    QPushButton* connect_button;
    QLineEdit* broker_url;
    QSpinBox* broker_port;
    QLineEdit* username;
    QLineEdit* password;
    QLineEdit* client_id;
    QLineEdit* base_topic;
    QCheckBox* auto_connect;
};