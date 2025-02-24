#include "plugin/OpenRGB2MQTT.h"
#include "mqtt/MQTTHandler.h"
#include "devices/DeviceManager.h"
#include "config/ConfigManager.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QDebug>

OpenRGB2MQTT::OpenRGB2MQTT() :
    dark_theme(false),
    resource_manager(nullptr),
    mqtt_handler(nullptr),
    device_manager(nullptr),
    config_manager(nullptr),
    main_widget(nullptr),
    components_initialized(false)
{
}

OpenRGB2MQTT::~OpenRGB2MQTT()
{
    cleanup();
}

OpenRGBPluginInfo OpenRGB2MQTT::GetPluginInfo()
{
    OpenRGBPluginInfo info;
    info.Name = "OpenRGB2MQTT";
    info.Description = "MQTT integration for OpenRGB";
    info.Version = VERSION_STRING;
    info.Commit = "N/A";
    info.URL = "https://github.com/yourusername/OpenRGB2MQTT";
    info.Location = OPENRGB_PLUGIN_LOCATION_SETTINGS;
    info.Label = "OpenRGB2MQTT";

    return info;
}

unsigned int OpenRGB2MQTT::GetPluginAPIVersion()
{
    return OPENRGB_PLUGIN_API_VERSION;
}

void OpenRGB2MQTT::Load(ResourceManagerInterface* resource_manager_ptr)
{
    QMutexLocker locker(&init_mutex);
    
    if (!resource_manager_ptr) {
        return;
    }

    resource_manager = resource_manager_ptr;

    try {
        // Initialize components with checks
        config_manager = new ConfigManager(this);
        if (!config_manager) {
            throw std::runtime_error("Failed to create ConfigManager");
        }

        mqtt_handler = new MQTTHandler(this);
        if (!mqtt_handler) {
            throw std::runtime_error("Failed to create MQTTHandler");
        }

        device_manager = new DeviceManager(resource_manager, this);
        if (!device_manager) {
            throw std::runtime_error("Failed to create DeviceManager");
        }

        // Create GUI components
        createMainWidget();

        // Schedule delayed initialization
        QTimer::singleShot(500, this, &OpenRGB2MQTT::delayedInit);

    } catch (const std::exception& e) {
        qDebug() << "[OpenRGB2MQTT] Initialization error:" << e.what();
        cleanup();
    }
}

void OpenRGB2MQTT::delayedInit()
{
    QMutexLocker locker(&init_mutex);
    
    try {
        // Connect signals/slots with Direct connection for thread safety
        connect(mqtt_handler, &MQTTHandler::messageReceived,
                device_manager, &DeviceManager::handleMQTTMessage,
                Qt::DirectConnection);

        connect(device_manager, &DeviceManager::mqttPublishNeeded,
                this, [this](const QString& topic, const QByteArray& payload) {
                    if (mqtt_handler) {
                        mqtt_handler->publish(topic, payload);
                    }
                }, Qt::QueuedConnection);

        // Mark initialization complete
        components_initialized = true;

        // Start auto-connect timer if enabled
        if (config_manager && config_manager->getAutoConnect()) {
            QTimer::singleShot(1000, this, [this]() {
                QMutexLocker autolock(&init_mutex);
                if (!components_initialized || !mqtt_handler || !config_manager) {
                    return;
                }
                initializeConnection();
            });
        }

    } catch (const std::exception& e) {
        qDebug() << "[OpenRGB2MQTT] Delayed initialization error:" << e.what();
        cleanup();
    }
}

QWidget* OpenRGB2MQTT::GetWidget()
{
    return main_widget;
}

QMenu* OpenRGB2MQTT::GetTrayMenu()
{
    return nullptr;
}

void OpenRGB2MQTT::Unload()
{
    if (mqtt_handler)
    {
        mqtt_handler->disconnect();
    }
}

void OpenRGB2MQTT::createMainWidget()
{
    if (main_widget) {
        return;
    }

    main_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(main_widget);

    // Create form layout for settings
    QGridLayout* form = new QGridLayout();
    int row = 0;

    // Broker URL
    form->addWidget(new QLabel("Broker URL:"), row, 0);
    broker_url = new QLineEdit();
    broker_url->setText(config_manager->getBrokerUrl());
    form->addWidget(broker_url, row++, 1);

    // Broker Port
    form->addWidget(new QLabel("Port:"), row, 0);
    broker_port = new QSpinBox();
    broker_port->setRange(1, 65535);
    broker_port->setValue(config_manager->getBrokerPort());
    form->addWidget(broker_port, row++, 1);

    // Username
    form->addWidget(new QLabel("Username:"), row, 0);
    username = new QLineEdit();
    username->setText(config_manager->getBrokerUsername());
    form->addWidget(username, row++, 1);

    // Password
    form->addWidget(new QLabel("Password:"), row, 0);
    password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);
    password->setText(config_manager->getBrokerPassword());
    form->addWidget(password, row++, 1);

    // Client ID
    form->addWidget(new QLabel("Client ID:"), row, 0);
    client_id = new QLineEdit();
    client_id->setText(config_manager->getClientId());
    form->addWidget(client_id, row++, 1);

    // Base Topic
    form->addWidget(new QLabel("Base Topic:"), row, 0);
    base_topic = new QLineEdit();
    base_topic->setText(config_manager->getBaseTopic());
    form->addWidget(base_topic, row++, 1);

    // Auto Connect
    auto_connect = new QCheckBox("Auto Connect");
    auto_connect->setChecked(config_manager->getAutoConnect());
    form->addWidget(auto_connect, row++, 1);

    // Status and Connect button
    QHBoxLayout* status_layout = new QHBoxLayout();
    status_label = new QLabel("Disconnected");
    connect_button = new QPushButton("Connect");

    status_layout->addWidget(status_label);
    status_layout->addWidget(connect_button);

    // Connect button handler
    connect(connect_button, &QPushButton::clicked, this, [this]() {
        if (!mqtt_handler->isConnected()) {
            initializeConnection();
        } else {
            mqtt_handler->disconnect();
            connect_button->setText("Connect");
            status_label->setText("Disconnected");
        }
    });

    // Add all layouts
    layout->addLayout(form);
    layout->addLayout(status_layout);
    layout->addStretch();

    // Connect MQTT status changes
    connect(mqtt_handler, &MQTTHandler::connectionStatusChanged,
            this, [this](bool connected) {
                connect_button->setText(connected ? "Disconnect" : "Connect");
                status_label->setText(connected ? "Connected" : "Disconnected");
            });

    connect(mqtt_handler, &MQTTHandler::connectionError,
            this, [this](const QString& error) {
                status_label->setText(QString("Error: %1").arg(error));
            });
}

void OpenRGB2MQTT::initializeConnection()
{
    if (!mqtt_handler || !config_manager) {
        return;
    }

    // Save current settings
    config_manager->setBrokerUrl(broker_url->text());
    config_manager->setBrokerPort(broker_port->value());
    config_manager->setBrokerUsername(username->text());
    config_manager->setBrokerPassword(password->text());
    config_manager->setClientId(client_id->text());
    config_manager->setBaseTopic(base_topic->text());
    config_manager->setAutoConnect(auto_connect->isChecked());

    // Set up will message
    mqtt_handler->setWillMessage(
        QString("%1/openrgb/status").arg(base_topic->text()),
        "offline"
    );

    // Connect to broker
    mqtt_handler->connectToHost(
        broker_url->text(),
        broker_port->value(),
        username->text(),
        password->text()
    );
}

void OpenRGB2MQTT::cleanup()
{
    if (mqtt_handler) {
        mqtt_handler->disconnect();
        delete mqtt_handler;
        mqtt_handler = nullptr;
    }
    
    if (device_manager) {
        delete device_manager;
        device_manager = nullptr;
    }
    
    if (config_manager) {
        delete config_manager;
        config_manager = nullptr;
    }
    
    if (main_widget) {
        delete main_widget;
        main_widget = nullptr;
    }

    components_initialized = false;
}