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
#include <QTabWidget>
#include <QGroupBox>
#include <QTableWidget>
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

private slots:
    // MQTT UI slots
    void onConnectButtonClicked();
    void onAutoConnectChanged(int state);
    
    // DDP UI slots
    void onDDPEnabledChanged(int state);
    void onDDPDiscoverButtonClicked();
    void onDDPAddDeviceButtonClicked();
    void onDDPRemoveDeviceButtonClicked();
    void onDDPDeviceSelectionChanged();
    void onDDPDiscoveryFinished(int count);
    
    // ESPHome UI slots
    void onESPHomeEnabledChanged(int state);
    
    // Zigbee UI slots
    void onZigbeeRefreshClicked();
    
    // Devices UI slots
    void onDeviceActionButtonClicked();
    void refreshDeviceList();
    void updateAllDevicesTable();

private:
    void createMainWidget();
    void createMQTTTab(QTabWidget* tabWidget);
    void createDDPTab(QTabWidget* tabWidget);
    void createESPHomeTab(QTabWidget* tabWidget);
    void createZigbeeTab(QTabWidget* tabWidget);
    void createDevicesTab(QTabWidget* tabWidget);
    void initializeConnection();
    void updateMQTTStatus(bool connected);
    void updateDDPStatus(bool enabled);
    void updateDDPDeviceList();
    void updateESPHomeStatus(bool enabled);
    void updateESPHomeDeviceList();
    void updateZigbeeStatus(bool enabled);
    void updateZigbeeDeviceList();
    void cleanup();
    
    bool dark_theme;
    ResourceManagerInterface* resource_manager;
    class MQTTHandler* mqtt_handler;
    class DeviceManager* device_manager;
    class DDPDeviceManager* ddp_manager;
    class ConfigManager* config_manager;
    QWidget* main_widget;
    QMutex init_mutex;
    bool components_initialized;
    QTimer* post_discovery_timer;  // Added to track the timer for cleanup
    
    // MQTT UI elements
    QWidget* mqtt_tab;
    QLabel* mqtt_status_label;
    QPushButton* connect_button;
    QLineEdit* broker_url;
    QSpinBox* broker_port;
    QLineEdit* username;
    QLineEdit* password;
    QLineEdit* client_id;
    QLineEdit* base_topic;
    QCheckBox* auto_connect;
    
    // DDP UI elements
    QWidget* ddp_tab;
    QLabel* ddp_status_label;
    QCheckBox* ddp_enabled;
    QTableWidget* ddp_device_table;
    QPushButton* ddp_discover_button;
    QPushButton* ddp_add_button;
    QPushButton* ddp_remove_button;
    QLineEdit* ddp_device_name;
    QLineEdit* ddp_device_ip;
    QSpinBox* ddp_device_leds;
    QLabel* ddp_discovery_status;
    
    // ESPHome UI elements
    QWidget* esphome_tab;
    QLabel* esphome_status_label;
    QCheckBox* esphome_enabled;
    QTableWidget* esphome_device_table;
    
    // Zigbee UI elements
    QWidget* zigbee_tab;
    QLabel* zigbee_status_label;
    QTableWidget* zigbee_device_table;
    QPushButton* zigbee_refresh_button;
    
    // Devices UI elements
    QWidget* devices_tab;
    QTableWidget* all_devices_table;
    QPushButton* refresh_devices_button;
};