# OpenRGB2MQTT Plugin for OpenRGB

OpenRGB2MQTT is a plugin that integrates OpenRGB with MQTT, allowing you to control your RGB devices through MQTT brokers and connect with various smart home platforms.

## Project Background

This project aims to bridge the gap between OpenRGB and smart home automation by providing a robust MQTT integration. It supports multiple protocols including:
- Native MQTT
- Zigbee (via MQTT bridge)
- ESPHome API and MQTT - Not working just yet

## Development Note

This plugin was created using assistance from:
- [Claude AI](https://claude.ai)

As such:
- Consider this a beta version
- Some features need refinement
- Bug reports are welcome
- Contributions are encouraged

## Known Issues

- Some times crashes on loading plugin. Running OpenRGB again seem to work.
- If you have Ikea Bulbs they will be slow to update. There is no way to fix this. Suggest using Philips Hue, LIFX or newer bulbs with Zigbee 3.0
- Tested with Zigbee2MQTT and Home Assistant Mosquitto Core. Can't guarentee it will work with every MQTT RGB or Zigbee devices.
- ESPHome doesn't work at this stage. Keep getting lots of loading erros and crashing. Still working on It


### Required DLLs
When distributing, ensure these DLLs are present in the main OpenRGB folder:

- Qt5Network.dll
- Qt5Mqtt.dll

You will need to setup QtMQTT in QT Creator if you want to compile your own versions

### Setting up QtMQTT for Qt 5.15.0

You will need to copy the contents of the MQTT QT Setup folder to your QT folder

C:\Qt\5.15.0\msvc2019_64

Restart you Qt Creator if it was running and this should register the module.

## Installation

1. Download the plugin file (OpenRGB2MQTT.dll)
2. Place it in your OpenRGB plugins folder
3. Run OpenRGB
4. Configure your MQTT broker settings in the plugin interface


