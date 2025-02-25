#-----------------------------------------------------------------------------------------------#
# OpenRGB2MQTT Plugin QMake Project                                                             #
#-----------------------------------------------------------------------------------------------#
include(mqtt_qt_setup/mkspecs/modules/qt_lib_mqtt.pri)

#-----------------------------------------------------------------------------------------------#
# Qt Configuration                                                                              #
#-----------------------------------------------------------------------------------------------#
QT += core gui network widgets mqtt

CONFIG += qmake_use

DEFINES += OPENRGB2MQTT_LIBRARY
DEFINES += OPENRGB2MQTT_PLUGIN_LIBRARY
TEMPLATE = lib

#-----------------------------------------------------------------------------------------------#
# Build Configuration                                                                           #
#-----------------------------------------------------------------------------------------------#
CONFIG += \
    plugin \
    silent \
    force_debug_info \

win32:CONFIG(debug, debug|release) {
    TARGET = OpenRGB2MQTTd
    LIBS += -L"C:/Qt5.15.0/5.15.0/msvc2019_64/lib" -lQt5Mqttd
    OBJECTS_DIR = $PWD/build/intermediate/debug/obj
    MOC_DIR = $PWD/build/intermediate/debug/moc
    RCC_DIR = $PWD/build/intermediate/debug/rcc
    UI_DIR = $PWD/build/intermediate/debug/ui
    PDB_DIR = $PWD/build/intermediate/debug
    # Build directories
    DESTDIR = "D:/MCP/OpenRGB2MQTT/build/output"
    # Compiler settings
    QMAKE_CXXFLAGS_RELEASE += /FS /MD /permissive- /Fd$shell_quote($PWD/build/intermediate/release/OpenRGB2MQTT.pdb)
    QMAKE_CXXFLAGS_DEBUG += /FS /MDd /permissive- /Fd$shell_quote($PWD/build/intermediate/debug/OpenRGB2MQTT.pdb)
    QMAKE_LFLAGS += /MACHINE:X64
} else {
    TARGET = OpenRGB2MQTT
    OBJECTS_DIR = $PWD/build/intermediate/release/obj
    MOC_DIR = $PWD/build/intermediate/release/moc
    RCC_DIR = $PWD/build/intermediate/release/rcc
    UI_DIR = $PWD/build/intermediate/release/ui
    PDB_DIR = $PWD/build/intermediate/release
    LIBS += -lQt5Mqtt
}

#-----------------------------------------------------------------------------------------------#
# Application Configuration                                                                     #
#-----------------------------------------------------------------------------------------------#
MAJOR           = 0
MINOR           = 1
REVISION        = 0
PLUGIN_VERSION  = $$MAJOR"."$$MINOR$$REVISION

#-----------------------------------------------------------------------------------------------#
# Version Information                                                                           #
#-----------------------------------------------------------------------------------------------#
DEFINES +=                                                                                      \
    VERSION_STRING=\\"\"\"$$PLUGIN_VERSION\\"\"\"                                               \

#-----------------------------------------------------------------------------------------------#
# Plugin Metadata                                                                               #
#-----------------------------------------------------------------------------------------------#
OTHER_FILES += OpenRGB2MQTT.json

#-----------------------------------------------------------------------------------------------#
# Includes                                                                                      #
#-----------------------------------------------------------------------------------------------#
HEADERS +=                                                                                      \
    src/utils/EncryptionHelper.h                                                                   \
    src/plugin/OpenRGB2MQTT.h                                                                   \
    src/mqtt/MQTTHandler.h                                                                      \
    src/devices/DeviceManager.h                                                                 \
    src/config/ConfigManager.h                                                                  \
    src/devices/base/MQTTRGBDevice.h                                                           \
    src/devices/base/CustomRGBController.h                                                     \
    src/devices/base/RGBControllerTypes.h                                                      \
    src/devices/mosquitto/MosquittoDeviceManager.h                                             \
    src/devices/mosquitto/MosquittoLightDevice.h                                               \
    src/devices/zigbee/ZigbeeDeviceManager.h                                                   \
    src/devices/zigbee/ZigbeeLightDevice.h                                                     \
    src/devices/esphome/ESPHomeDeviceManager.h                                                 \
    src/devices/esphome/ESPHomeLightDevice.h                                                   \
    src/devices/esphome/ESPHomeAPIDevice.h                                                    \
    src/devices/esphome/ESPHomeAPIManager.h                                                   \
    OpenRGB/RGBController/RGBController.h                                                       \
    OpenRGB/RGBController/RGBControllerKeyNames.h                                               \

SOURCES +=                                                                                      \
    src/utils/EncryptionHelper.cpp                                                                 \
    src/plugin/OpenRGB2MQTT.cpp                                                                 \
    src/mqtt/MQTTHandler.cpp                                                                    \
    src/devices/DeviceManager.cpp                                                               \
    src/config/ConfigManager.cpp                                                                \
    src/devices/base/MQTTRGBDevice.cpp                                                         \
    src/devices/base/CustomRGBController.cpp                                                   \
    src/devices/mosquitto/MosquittoDeviceManager.cpp                                           \
    src/devices/mosquitto/MosquittoLightDevice.cpp                                             \
    src/devices/zigbee/ZigbeeDeviceManager.cpp                                                 \
    src/devices/zigbee/ZigbeeLightDevice.cpp                                                   \
    src/devices/esphome/ESPHomeDeviceManager.cpp                                               \
    src/devices/esphome/ESPHomeLightDevice.cpp                                                 \
    src/devices/esphome/ESPHomeAPIDevice.cpp                                                  \
    src/devices/esphome/ESPHomeAPIManager.cpp                                                 \
    OpenRGB/RGBController/RGBController.cpp                                                     \
    OpenRGB/RGBController/RGBControllerKeyNames.cpp                                             \

RESOURCES +=                                                                                    \
    resources/resources.qrc                                                                      \

#-----------------------------------------------------------------------------------------------#
# OpenRGB Plugin SDK                                                                            #
#-----------------------------------------------------------------------------------------------#
INCLUDEPATH +=                                                                                  \
    mqtt_qt_setup/include/                                                                      \
    src/                                                                                        \
    src/devices/base                                                                            \
    src/devices/mosquitto                                                                       \
    src/devices/zigbee                                                                          \
    src/devices/esphome                                                                         \
    OpenRGB/                                                                                    \
    OpenRGB/i2c_smbus                                                                           \
    OpenRGB/RGBController                                                                       \
    OpenRGB/net_port                                                                            \
    OpenRGB/dependencies/json                                                                   \
    OpenRGB/hidapi_wrapper                                                                      \
    OpenRGB/dependencies/hidapi-win/include                                                     \
    OpenRGB/SPDAccessor                                                                         \

#-----------------------------------------------------------------------------------------------#
# Windows-specific Configuration                                                                #
#-----------------------------------------------------------------------------------------------#
win32:CONFIG += QTPLUGIN c++17

win32:contains(QMAKE_TARGET.arch, x86_64) {
    LIBS +=                                                                                     \
        -lws2_32                                                                                \
        -lole32                                                                                 \
}

win32:HEADERS += \
    src/mqtt/MQTTHandler.h \

win32:SOURCES += \
    src/mqtt/MQTTHandler.cpp \

win32:DEFINES +=                                                                                \
    _MBCS                                                                                       \

#-----------------------------------------------------------------------------------------------#
# Linux-specific Configuration                                                                  #
#-----------------------------------------------------------------------------------------------#
unix:!macx {
    QMAKE_CXXFLAGS += -std=c++17
    target.path=$$PREFIX/lib/openrgb/plugins/
    INSTALLS += target
}

#-----------------------------------------------------------------------------------------------#
# MacOS-specific Configuration                                                                  #
#-----------------------------------------------------------------------------------------------#
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

macx: {
    CONFIG += c++17
}