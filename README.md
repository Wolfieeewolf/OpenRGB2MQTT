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

## Building 

OpenRGB2MQTT uses the OpenRGB submodule you will have to clone that into the OpenRGB2MQTT directory



## Installation

1. Download the plugin file (OpenRGB2MQTT.dll)
2. Place it in your OpenRGB plugins folder
3. Run OpenRGB
4. Configure your MQTT broker settings in the plugin interface


## Open Source and Collaboration

Feel free to fork this repository and modify it however you like! This project is meant to be:
- An experiment in AI-assisted development
- A learning resource for others
- A foundation for community improvements
- Open to any and all modifications

If you're interested in the intersection of AI and development, this project serves as a real-world example of what's possible.

## Development Status

This plugin is in very early stage. Expect bugs and incomplete features. Please report any issues or feature requests through GitHub issues, but understand that as this is an AI-assisted project, fixes and updates may take time as we experiment with the best ways to implement changes.


## Supporting Development

This project is developed using AI assistance, which involves subscription and API costs. If you find Lightscape useful and would like to support its continued development, you're welcome to contribute via:



<a href="https://www.buymeacoffee.com/Wolfieee"><img src="https://img.buymeacoffee.com/button-api/?text=Buy me a pizza&emoji=🍕&slug=Wolfieee&button_colour=40DCA5&font_colour=ffffff&font_family=Poppins&outline_colour=000000&coffee_colour=FFDD00" /></a>



Your support helps maintain the AI tools that make this development possible, but the plugin will always remain free and open source regardless.



## License

This project is open-source software. Feel free to experiment, modify, and improve upon it!
