# OpenRGB2MQTT Plugin QMake Project
include(mqtt_qt_setup/mkspecs/modules/qt_lib_mqtt.pri)

QT += core gui network widgets
CONFIG += plugin silent c++17
TEMPLATE = lib
TARGET = OpenRGB2MQTT

# Define Git commit information for LogManager
DEFINES += GIT_COMMIT_ID=\\\"plugin-build\\\"
DEFINES += GIT_COMMIT_DATE=\\\"plugin-date\\\"

# Output directories
DESTDIR = $$PWD/build/output
OBJECTS_DIR = $$PWD/build/intermediate/obj
MOC_DIR = $$PWD/build/intermediate/moc
RCC_DIR = $$PWD/build/intermediate/rcc
UI_DIR = $$PWD/build/intermediate/ui

# Library dependencies
win32 {
    LIBS += -L$$PWD/mqtt_qt_setup/lib/ -lQt5Mqtt -lws2_32 -lole32
    DEFINES += WIN32 _CRT_SECURE_NO_WARNINGS USE_HID_USAGE
}
unix:!macx {
    LIBS += -lQt5Mqtt
    QMAKE_CXXFLAGS += -std=c++17 -Wno-psabi
    target.path=/usr/local/lib/openrgb/plugins/
    INSTALLS += target
}
macx {
    LIBS += -lQt5Mqtt
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
}

# Version info
MAJOR = 0
MINOR = 2
REVISION = 0
PLUGIN_VERSION = $$MAJOR"."$$MINOR$$REVISION
DEFINES += VERSION_STRING=\\\"$$PLUGIN_VERSION\\\"

# Plugin metadata
OTHER_FILES += OpenRGB2MQTT.json

# Include paths
INCLUDEPATH += \
    mqtt_qt_setup/include \
    src/ \
    src/devices/base \
    src/devices/mosquitto \
    OpenRGB/ \
    OpenRGB/i2c_smbus \
    OpenRGB/RGBController \
    OpenRGB/net_port \
    OpenRGB/dependencies/json \
    OpenRGB/hidapi_wrapper \
    OpenRGB/dependencies/hidapi-win/include \
    OpenRGB/SPDAccessor \

DEPENDPATH += mqtt_qt_setup/include OpenRGB/RGBController

# Source files
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
    OpenRGB/RGBController/RGBController.h \
    OpenRGB/RGBController/RGBControllerKeyNames.h \
    OpenRGB/LogManager.h

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
    OpenRGB/RGBController/RGBController.cpp \
    OpenRGB/RGBController/RGBControllerKeyNames.cpp \
    OpenRGB/LogManager.cpp

RESOURCES += resources/resources.qrc

# Post-build step for Windows
win32 {
    QMAKE_POST_LINK = cmd /c \
        if not exist \"%APPDATA%\\OpenRGB\\plugins\" mkdir \"%APPDATA%\\OpenRGB\\plugins\" && \
        copy /Y \"$$shell_path($$DESTDIR/$$TARGET).dll\" \"%APPDATA%\\OpenRGB\\plugins\"
}