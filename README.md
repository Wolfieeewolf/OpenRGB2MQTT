# OpenRGB2MQTT Plugin for OpenRGB

OpenRGB2MQTT is a plugin that integrates OpenRGB with MQTT, allowing you to control your RGB devices through MQTT brokers and connect with various smart home platforms.

## Project Background

This project aims to bridge the gap between OpenRGB and smart home automation by providing a robust MQTT integration.

## Features

- **Cross-Platform Support**: Works on Windows, Linux, and macOS
- **Auto-Discovery**: Automatically finds compatible RGB devices on your network
- **Device Management**: Add or remove devices without restarting OpenRGB
- **Live Updates**: Changes take effect immediately
- **Configuration Persistence**: Settings are automatically saved
- **Auto-Connect**: Option to automatically connect on startup

## Development Note

This plugin was created using assistance from:
- [Claude AI](https://claude.ai)

As such:
- Consider this a beta version
- Some features need refinement
- Bug reports are welcome
- Contributions are encouraged

## Known Issues

- Sometimes crashes on loading plugin. Running OpenRGB again seems to work.
- Tested with Home Assistant Mosquitto Core. Can't guarantee it will work with every MQTT RGB device.

## MQTT Integration

Connect with MQTT brokers to discover and control MQTT-compatible devices.
- Supports username/password authentication
- Configurable base topic
- Automatic discovery of MQTT RGB devices
- Compatible with Home Assistant MQTT integration

## Required DLLs
When distributing, ensure these DLLs are present in the main OpenRGB folder:

- Qt5Network.dll
- Qt5Mqtt.dll

You will need to setup QtMQTT in Qt Creator if you want to compile your own versions

### Setting up QtMQTT for Qt 5.15.0

You will need to copy the contents of the MQTT QT Setup folder to your Qt folder

C:\Qt\5.15.0\msvc2019_64

Restart your Qt Creator if it was running and this should register the module.

## Building 

OpenRGB2MQTT uses the OpenRGB submodule. You will have to clone that into the OpenRGB2MQTT directory.

```bash
git clone --recursive https://github.com/yourusername/OpenRGB2MQTT.git
cd OpenRGB2MQTT
```

If you've already cloned the repository without the `--recursive` flag:

```bash
git submodule update --init --recursive
```

## Installation

1. Download the plugin file (OpenRGB2MQTT.dll for Windows, libOpenRGB2MQTT.so for Linux)
2. Copy Qt5Mqtt.dll and Qt5Network.dll to your OpenRGB folder. Place in folder with the OpenRGB.exe
3. Place it in your OpenRGB plugins folder
4. Run OpenRGB
5. Configure your MQTT broker settings in the plugin interface
6. Discover and manage your RGB devices through the plugin's interface

## Open Source and Collaboration

Feel free to fork this repository and modify it however you like! This project is meant to be:
- An experiment in AI-assisted development
- A learning resource for others
- A foundation for community improvements
- Open to any and all modifications

If you're interested in the intersection of AI and development, this project serves as a real-world example of what's possible.

## Development Status

This plugin is stable for MQTT integration.

## Supporting Development

This project is developed using AI assistance, which involves subscription and API costs. If you find OpenRGB2MQTT useful and would like to support its continued development, you're welcome to contribute via:

<a href="https://www.buymeacoffee.com/Wolfieee"><img src="https://img.buymeacoffee.com/button-api/?text=Buy me a pizza&emoji=🍕&slug=Wolfieee&button_colour=40DCA5&font_colour=ffffff&font_family=Poppins&outline_colour=000000&coffee_colour=FFDD00" /></a>

Your support helps maintain the AI tools that make this development possible, but the plugin will always remain free and open source regardless.

## License

This project is open-source software. Feel free to experiment, modify, and improve upon it!