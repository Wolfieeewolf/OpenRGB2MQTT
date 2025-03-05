#-----------------------------------------------------------------------------------------------#
# OpenRGB2MQTT Plugin QMake Project                                                             #
#-----------------------------------------------------------------------------------------------#

# Include MQTT module pri file (for reference, may not be needed with direct lib linking)
include(mqtt_qt_setup/mkspecs/modules/qt_lib_mqtt.pri)

#-----------------------------------------------------------------------------------------------#
# Qt Configuration                                                                              #
#-----------------------------------------------------------------------------------------------#
QT += core gui network widgets

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

# Build directories
# We'll build directly to output folder to avoid post-build copy issues
win32 {
    OUTPUT_DIR = $$PWD/build/output
    DESTDIR = $$OUTPUT_DIR
}
unix:!macx {
    DESTDIR = $$PWD/build/output
}

# Intermediate build folders
OBJECTS_DIR = $$PWD/build/intermediate/obj
MOC_DIR = $$PWD/build/intermediate/moc
RCC_DIR = $$PWD/build/intermediate/rcc
UI_DIR = $$PWD/build/intermediate/ui

# Compiler settings
win32 {
    QMAKE_CXXFLAGS_RELEASE += /FS /MD /permissive- /Fd$$shell_quote($$PWD/build/intermediate/release/OpenRGB2MQTT.pdb)
    QMAKE_CXXFLAGS_DEBUG += /FS /MDd /permissive- /Fd$$shell_quote($$PWD/build/intermediate/debug/OpenRGB2MQTT.pdb)
    QMAKE_LFLAGS += /MACHINE:X64
}

CONFIG(debug, debug|release) {
    TARGET = OpenRGB2MQTTd
    OBJECTS_DIR = $$PWD/build/intermediate/debug/obj
    MOC_DIR = $$PWD/build/intermediate/debug/moc
    RCC_DIR = $$PWD/build/intermediate/debug/rcc
    UI_DIR = $$PWD/build/intermediate/debug/ui
    PDB_DIR = $$PWD/build/intermediate/debug
    
    win32 {
        # For debug builds, use the release library since debug version isn't available
        LIBS += -L$$PWD/mqtt_qt_setup/lib/ -lQt5Mqtt
    }
    unix:!macx {
        LIBS += -lQt5Mqtt
    }
} else {
    TARGET = OpenRGB2MQTT
    OBJECTS_DIR = $$PWD/build/intermediate/release/obj
    MOC_DIR = $$PWD/build/intermediate/release/moc
    RCC_DIR = $$PWD/build/intermediate/release/rcc
    UI_DIR = $$PWD/build/intermediate/release/ui
    PDB_DIR = $$PWD/build/intermediate/release
    
    win32 {
        # For release builds, use the release library
        LIBS += -L$$PWD/mqtt_qt_setup/lib/ -lQt5Mqtt
    }
    unix:!macx {
        LIBS += -lQt5Mqtt
    }
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
DEFINES += \
    VERSION_STRING=\\"\"\"$$PLUGIN_VERSION\\"\"\" \

#-----------------------------------------------------------------------------------------------#
# Plugin Metadata                                                                               #
#-----------------------------------------------------------------------------------------------#
OTHER_FILES += OpenRGB2MQTT.json

#-----------------------------------------------------------------------------------------------#
# Includes                                                                                      #
#-----------------------------------------------------------------------------------------------#
HEADERS += \
    src/utils/EncryptionHelper.h \
    src/plugin/OpenRGB2MQTT.h \
    src/mqtt/MQTTHandler.h \
    src/devices/DeviceManager.h \
    src/config/ConfigManager.h \
    src/devices/base/MQTTRGBDevice.h \
    src/devices/base/CustomRGBController.h \
    src/devices/base/RGBControllerTypes.h \
    src/devices/mosquitto/MosquittoDeviceManager.h \
    src/devices/mosquitto/MosquittoLightDevice.h \
    src/devices/zigbee/ZigbeeDeviceManager.h \
    src/devices/zigbee/ZigbeeLightDevice.h \
    src/devices/esphome/ESPHomeDeviceManager.h \
    src/devices/esphome/ESPHomeLightDevice.h \
    src/devices/esphome/ESPHomeAPIDevice.h \
    src/devices/esphome/ESPHomeAPIManager.h \
    src/devices/ddp/DDPController.h \
    src/devices/ddp/DDPDeviceManager.h \
    src/devices/ddp/DDPLightDevice.h \
    OpenRGB/RGBController/RGBController.h \
    OpenRGB/RGBController/RGBControllerKeyNames.h \

SOURCES += \
    src/utils/EncryptionHelper.cpp \
    src/plugin/OpenRGB2MQTT.cpp \
    src/mqtt/MQTTHandler.cpp \
    src/devices/DeviceManager.cpp \
    src/config/ConfigManager.cpp \
    src/devices/base/MQTTRGBDevice.cpp \
    src/devices/base/CustomRGBController.cpp \
    src/devices/mosquitto/MosquittoDeviceManager.cpp \
    src/devices/mosquitto/MosquittoLightDevice.cpp \
    src/devices/zigbee/ZigbeeDeviceManager.cpp \
    src/devices/zigbee/ZigbeeLightDevice.cpp \
    src/devices/esphome/ESPHomeDeviceManager.cpp \
    src/devices/esphome/ESPHomeLightDevice.cpp \
    src/devices/esphome/ESPHomeAPIDevice.cpp \
    src/devices/esphome/ESPHomeAPIManager.cpp \
    src/devices/ddp/DDPController.cpp \
    src/devices/ddp/DDPDeviceManager.cpp \
    src/devices/ddp/DDPLightDevice.cpp \
    OpenRGB/RGBController/RGBController.cpp \
    OpenRGB/RGBController/RGBControllerKeyNames.cpp \

RESOURCES += \
    resources/resources.qrc \

#-----------------------------------------------------------------------------------------------#
# OpenRGB Plugin SDK                                                                            #
#-----------------------------------------------------------------------------------------------#
INCLUDEPATH += \
    mqtt_qt_setup/include \
    src/ \
    src/devices/base \
    src/devices/mosquitto \
    src/devices/zigbee \
    src/devices/esphome \
    src/devices/ddp \
    OpenRGB/ \
    OpenRGB/i2c_smbus \
    OpenRGB/RGBController \
    OpenRGB/net_port \
    OpenRGB/dependencies/json \
    OpenRGB/hidapi_wrapper \
    OpenRGB/dependencies/hidapi-win/include \
    OpenRGB/SPDAccessor \

DEPENDPATH += \
    mqtt_qt_setup/include \
    OpenRGB/RGBController \

#-----------------------------------------------------------------------------------------------#
# Windows-specific Configuration                                                                #
#-----------------------------------------------------------------------------------------------#
win32:CONFIG += QTPLUGIN c++17

win32:contains(QMAKE_TARGET.arch, x86_64) {
    LIBS += \
        -lws2_32 \
        -lole32 \
}

win32:DEFINES += \
    _MBCS \
    WIN32 \
    _CRT_SECURE_NO_WARNINGS \
    _WINSOCK_DEPRECATED_NO_WARNINGS \
    WIN32_LEAN_AND_MEAN \
    USE_HID_USAGE \

win32:QMAKE_CXXFLAGS += -wd4267  # Disable conversion size warnings

# Add manual post-build step to copy DLL to OpenRGB plugins directory
win32 {
    PLUGIN_DIR = C:/Users/wolfi/AppData/Roaming/OpenRGB/plugins
    
    # Create target directory and copy file
    QMAKE_POST_LINK = \
        if not exist \"$$PLUGIN_DIR\" mkdir \"$$PLUGIN_DIR\" & \
        copy /Y \"$$shell_path($$DESTDIR/$(TARGET))\" \"$$PLUGIN_DIR\"
}

#-----------------------------------------------------------------------------------------------#
# Linux-specific Configuration                                                                  #
#-----------------------------------------------------------------------------------------------#
unix:!macx {
    QMAKE_CXXFLAGS += -std=c++17 -Wno-psabi
    
    # Allow installation to system directories
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }
    target.path=$$PREFIX/lib/openrgb/plugins/
    INSTALLS += target

    # Additional Linux-specific headers & sources
    exists(src/mqtt/MQTTHandler_linux.h) {
        HEADERS += src/mqtt/MQTTHandler_linux.h
    }
    
    exists(src/mqtt/MQTTHandler_linux.cpp) {
        SOURCES += src/mqtt/MQTTHandler_linux.cpp
    }
}

#-----------------------------------------------------------------------------------------------#
# MacOS-specific Configuration                                                                  #
#-----------------------------------------------------------------------------------------------#
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

macx: {
    CONFIG += c++17
}