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
#include "OpenRGB/LogManager.h"
#include <QTabWidget>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QApplication>
#include <QNetworkInterface>
#include <QDialog>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

OpenRGB2MQTT::OpenRGB2MQTT() :
    dark_theme(false),
    resource_manager(nullptr),
    mqtt_handler(nullptr),
    device_manager(nullptr),
    config_manager(nullptr),
    main_widget(nullptr),
    components_initialized(false),
    post_discovery_timer(nullptr)
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
    info.URL = "https://github.com/wolfieeee/OpenRGB2MQTT";
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
        // Detect dark theme
        if (QApplication::palette().color(QPalette::Window).lightness() < 128) {
            dark_theme = true;
        }
        
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
        
        // Register managers with device manager
        device_manager->setConfigManager(config_manager);
        device_manager->setMQTTHandler(mqtt_handler);

        // Create GUI components
        createMainWidget();

        // Schedule delayed initialization
        QTimer::singleShot(500, this, &OpenRGB2MQTT::delayedInit);

    } catch (const std::exception& e) {
        LOG_WARNING("[OpenRGB2MQTT] Initialization error: %s", e.what());
        cleanup();
    }
}

void OpenRGB2MQTT::delayedInit()
{
    QMutexLocker locker(&init_mutex);
    
    try {
        // Connect signals/slots with Direct connection for thread safety
        if (mqtt_handler && device_manager) {
            // Direct connection for MQTT messages to ensure immediate processing
            connect(mqtt_handler, &MQTTHandler::messageReceived,
                    device_manager, &DeviceManager::handleMQTTMessage,
                    Qt::DirectConnection);
            
            LOG_INFO("Connected MQTT message signal to DeviceManager");
        }

        if (device_manager && mqtt_handler) {
            // Connect device manager to MQTT handler for publishing
            connect(device_manager, &DeviceManager::mqttPublishNeeded,
                    this, [this](const QString& topic, const QByteArray& payload) {
                        if (mqtt_handler) {
                            LOG_DEBUG("Publishing to MQTT topic: %s", qUtf8Printable(topic));
                            mqtt_handler->publish(topic, payload);
                        }
                    }, Qt::QueuedConnection);
                    
            LOG_INFO("Connected DeviceManager publish signal to MQTT handler");
        }
                


        // Mark initialization complete
        components_initialized = true;

        // Initialize all devices table
        QTimer::singleShot(1500, this, &OpenRGB2MQTT::updateAllDevicesTable);

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
        LOG_WARNING("[OpenRGB2MQTT] Delayed initialization error: %s", e.what());
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
    // First disconnect MQTT to prevent any new messages from coming in
    if (mqtt_handler) {
        mqtt_handler->disconnect();
    }
    


    // Clear QTimer connections to prevent delayed signals from firing during cleanup
    if (post_discovery_timer) {
        post_discovery_timer->disconnect();
        post_discovery_timer->stop();
    }

    // Block signals to prevent crashes from signal emission during destruction
    if (device_manager) {
        device_manager->blockSignals(true);
    }
    
    // Save device states with crash protection
    try {
        // Save the current device states to config manager before unloading
        if (device_manager && config_manager) {
            // Make a local copy of devices to prevent access to deleted objects
            std::vector<std::pair<std::string, std::string>> devices_copy;
            {
                // Get all devices and save their current state in a safe manner
                devices_copy = device_manager->getAllAvailableDevices();
            }
            
            // Process devices safely
            for (const auto& device_info : devices_copy) {
                const std::string& name = device_info.first;
                bool is_enabled = false;
                
                try {
                    // Get current device state with error handling
                    is_enabled = device_manager->isDeviceAddedToOpenRGB(name);
                    config_manager->setDeviceEnabled(name, is_enabled);
                }
                catch (...) {
                    // Silently ignore exceptions during unload
                }
            }

            // Force save ConfigManager state
            config_manager->saveConfig(config_manager->config_file);
        }
    }
    catch (...) {
        // Silently ignore exceptions during unload
    }
}

void OpenRGB2MQTT::createMainWidget()
{
    if (main_widget) {
        return;
    }

    main_widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(main_widget);
    
    // Create tab widget for all protocols and device management
    QTabWidget* tabWidget = new QTabWidget();
    
    // Create tabs - Devices tab first
    createDevicesTab(tabWidget);
    createMQTTTab(tabWidget);
    
    // Add tab widget to main layout
    layout->addWidget(tabWidget);
}

void OpenRGB2MQTT::createMQTTTab(QTabWidget* tabWidget)
{
    // Create MQTT tab
    mqtt_tab = new QWidget();
    QVBoxLayout* mqtt_layout = new QVBoxLayout(mqtt_tab);
    
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
    connect(auto_connect, SIGNAL(stateChanged(int)), this, SLOT(onAutoConnectChanged(int)));
    form->addWidget(auto_connect, row++, 1);

    // Status and Connect button
    QHBoxLayout* status_layout = new QHBoxLayout();
    mqtt_status_label = new QLabel("Disconnected");
    connect_button = new QPushButton("Connect");

    status_layout->addWidget(mqtt_status_label);
    status_layout->addWidget(connect_button);

    // Connect button handler
    connect(connect_button, &QPushButton::clicked, this, &OpenRGB2MQTT::onConnectButtonClicked);

    // Add all layouts
    mqtt_layout->addLayout(form);
    mqtt_layout->addLayout(status_layout);
    mqtt_layout->addStretch();

    // Connect MQTT status changes
    connect(mqtt_handler, &MQTTHandler::connectionStatusChanged,
            this, [this](bool connected) {
                updateMQTTStatus(connected);
            });

    connect(mqtt_handler, &MQTTHandler::connectionError,
            this, [this](const QString& error) {
                mqtt_status_label->setText(QString("Error: %1").arg(error));
            });
            
    // Add to tab widget
    tabWidget->addTab(mqtt_tab, "MQTT");
}







void OpenRGB2MQTT::createDevicesTab(QTabWidget* tabWidget)
{
    // Create All Devices tab
    devices_tab = new QWidget();
    QVBoxLayout* devices_layout = new QVBoxLayout(devices_tab);
    
    // Control buttons
    QHBoxLayout* control_layout = new QHBoxLayout();
    
    refresh_devices_button = new QPushButton("Refresh Devices");
    connect(refresh_devices_button, SIGNAL(clicked()), this, SLOT(refreshDeviceList()));
    
    control_layout->addWidget(refresh_devices_button);
    control_layout->addStretch();
    
    // Add control layout
    devices_layout->addLayout(control_layout);
    
    // Devices table
    all_devices_table = new QTableWidget(0, 4);
    all_devices_table->setHorizontalHeaderLabels({"Name", "Protocol", "Action", "Status"});
    all_devices_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    all_devices_table->setSelectionBehavior(QTableWidget::SelectRows);
    
    devices_layout->addWidget(all_devices_table);
    
    // Info label
    QLabel* info_label = new QLabel("Click the 'Add' button to add a device to OpenRGB. Click 'Remove' to remove it.");
    devices_layout->addWidget(info_label);
    
    // Success label
    QLabel* success_label = new QLabel("Changes take effect immediately - no restart required!");
    success_label->setStyleSheet("color: green;");
    devices_layout->addWidget(success_label);
    
    // Add to tab widget
    tabWidget->addTab(devices_tab, "MQTT Devices");
}

void OpenRGB2MQTT::onConnectButtonClicked()
{
    if (!mqtt_handler->isConnected()) {
        // Connect to MQTT
        initializeConnection();
    } else {
        // Disconnect from MQTT
        mqtt_handler->disconnect();
        connect_button->setText("Connect");
        mqtt_status_label->setText("Disconnected");
        
        // Also clear the devices since MQTT is disconnected
        if (device_manager) {
            // Temporarily remove all devices from OpenRGB while disconnected
            auto devices = device_manager->getAllAvailableDevices();
            for (const auto& device_info : devices) {
                const std::string& name = device_info.first;
                const std::string& protocol = device_info.second;
                
                // Only remove MQTT and Zigbee devices since they depend on MQTT
                if (protocol == "MQTT" || protocol == "Zigbee") {
                    // Remove device from OpenRGB temporarily
                    device_manager->addDeviceToOpenRGB(name, false);
                }
            }
        }
        
        // Refresh the devices tabs
        updateAllDevicesTable();
    }
}

void OpenRGB2MQTT::onAutoConnectChanged(int state)
{
    if (config_manager) {
        config_manager->setAutoConnect(state == Qt::Checked);
    }
}



void OpenRGB2MQTT::onDeviceActionButtonClicked()
{
    try {
        // Get the sender button
        QPushButton* button = qobject_cast<QPushButton*>(sender());
        if (!button) return;
        
        // Disable button immediately to prevent double-clicks
        button->setEnabled(false);
        
        // Find the row this button belongs to
        int row = -1;
        for (int i = 0; i < all_devices_table->rowCount(); i++) {
            QWidget* cellWidget = all_devices_table->cellWidget(i, 2);
            if (cellWidget) {
                QPushButton* cellButton = cellWidget->findChild<QPushButton*>();
                if (cellButton && cellButton == button) {
                    row = i;
                    break;
                }
            }
        }
        
        if (row == -1) {
            LOG_WARNING("Could not find button's row");
            button->setEnabled(true);
            return;
        }
        
        // Get device name safely
        QTableWidgetItem* nameItem = all_devices_table->item(row, 0);
        if (!nameItem) {
            LOG_WARNING("Could not find name item in row %d", row);
            button->setEnabled(true);
            return;
        }
        
        QString deviceName = nameItem->text();
        LOG_DEBUG("Device action for: %s", qUtf8Printable(deviceName));
        
        // Check current status to determine action
        bool currently_enabled = false;
        if (device_manager) {
            try {
                currently_enabled = device_manager->isDeviceAddedToOpenRGB(deviceName.toStdString());
            } catch (...) {
                LOG_WARNING("Error checking current device status");
                currently_enabled = false;
            }
        }
        
        // Toggle status
        bool new_status = !currently_enabled;
        
        // First update button text to provide immediate visual feedback
        button->setText(new_status ? "Remove" : "Add");
        
        // Update config first - this is always safe
        if (config_manager) {
            config_manager->setDeviceEnabled(deviceName.toStdString(), new_status);
        }
        
        // Update status text
        QTableWidgetItem* statusItem = all_devices_table->item(row, 3);
        if (!statusItem) {
            statusItem = new QTableWidgetItem();
            all_devices_table->setItem(row, 3, statusItem);
        }
        
        // Apply change to device manager if it exists
        bool success = false;
        if (device_manager) {
            // Try to update the device registry with crash protection
            try {
                success = device_manager->addDeviceToOpenRGB(deviceName.toStdString(), new_status);
            } catch (const std::exception& e) {
                LOG_ERROR("Exception when adding device to OpenRGB: %s", e.what());
                success = false;
            } catch (...) {
                LOG_ERROR("Unknown exception when adding device to OpenRGB");
                success = false;
            }
        }
        
        // Update UI based on success/failure
        if (success) {
            // Show success status
            statusItem->setText(new_status ? "Added to OpenRGB" : "Not in OpenRGB");
            
            // Done - the update has already been triggered in the device manager
            // No need to refresh the UI here as it can cause reentrant device updates
        } else {
            // Show error status
            statusItem->setText("Error - Config saved, restart OpenRGB");
            // If failed, revert button text
            button->setText(currently_enabled ? "Remove" : "Add");
        }
        
        // Re-enable button after a delay
        QTimer::singleShot(500, this, [button]() {
            if (button) button->setEnabled(true);
        });
    } catch (const std::exception& e) {
        LOG_ERROR("Critical exception in button handler: %s", e.what());
    } catch (...) {
        LOG_ERROR("Unknown critical exception in button handler");;
    }
}

void OpenRGB2MQTT::refreshDeviceList()
{
    updateAllDevicesTable();
}

void OpenRGB2MQTT::updateAllDevicesTable()
{
    if (!device_manager) return;
    
    // Clear the table
    all_devices_table->clearContents();
    all_devices_table->setRowCount(0);
    
    // Get devices from device manager
    auto all_available_devices = device_manager->getAllAvailableDevices();
    
    int row = 0;
    
    // Add all devices to the table
    for (const auto& device_info : all_available_devices) {
        const std::string& name = device_info.first;
        const std::string& protocol = device_info.second;
        
        all_devices_table->insertRow(row);
        
        QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(name));
        QTableWidgetItem* protocolItem = new QTableWidgetItem(QString::fromStdString(protocol));
        QTableWidgetItem* statusItem = new QTableWidgetItem();
        
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        protocolItem->setFlags(protocolItem->flags() & ~Qt::ItemIsEditable);
        statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
        
        // Add items to table
        all_devices_table->setItem(row, 0, nameItem);
        all_devices_table->setItem(row, 1, protocolItem);
        all_devices_table->setItem(row, 3, statusItem);
        
        // Create a button for the action column
        QWidget* buttonWidget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(buttonWidget);
        layout->setContentsMargins(2, 2, 2, 2);
        
        // Check if device is already added
        bool is_added = false;
        try {
            is_added = device_manager->isDeviceAddedToOpenRGB(name);
        } catch (...) {
            // If any exception occurs, default to not added
            is_added = false;
        }
        
        // Create appropriate button based on current status
        QPushButton* actionButton = new QPushButton(is_added ? "Remove" : "Add");
        connect(actionButton, SIGNAL(clicked()), this, SLOT(onDeviceActionButtonClicked()));
        
        layout->addWidget(actionButton);
        layout->setAlignment(Qt::AlignCenter);
        buttonWidget->setLayout(layout);
        
        // Add button widget to table
        all_devices_table->setCellWidget(row, 2, buttonWidget);
        
        // Set status text
        statusItem->setText(is_added ? "Added to OpenRGB" : "Not in OpenRGB");
        
        row++;
    }
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
    
    // Reset device UI state after connection attempt
    if (device_manager) {
        // Force a UI update and clear device list
        updateAllDevicesTable();
    }
}

void OpenRGB2MQTT::updateMQTTStatus(bool connected)
{
    connect_button->setText(connected ? "Disconnect" : "Connect");
    mqtt_status_label->setText(connected ? "Connected" : "Disconnected");
    
    if (connected) {
        // When connection is established, trigger discovery
        if (device_manager) {
            QTimer::singleShot(1000, this, [this]() {
                // Start device discovery 
                device_manager->discoverDevices();
                
                // Update UI after a brief delay to show newly discovered devices
                QTimer::singleShot(2000, this, [this]() {
                    updateAllDevicesTable();
                });
            });
        }
    }
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
