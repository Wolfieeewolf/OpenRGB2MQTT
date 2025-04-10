#include "DDPLightDevice.h"
#include "OpenRGB/LogManager.h"

DDPLightDevice::DDPLightDevice(const DeviceInfo& info)
    : controller(new DDPController(info.ip_address))
    , ip_address(info.ip_address)
    , num_leds(info.num_leds)
    , is_connected(false)
{
    // Set up RGBController properties
    name = info.name.toStdString();
    vendor = "DDP";
    type = DEVICE_TYPE_LEDSTRIP;
    description = info.device_type.toStdString() + " (DDP)";
    version = "";
    serial = info.ip_address.toStdString();
    location = "IP: " + info.ip_address.toStdString();
    
    // Set up modes
    modes.resize(1);
    modes[0].name = "Direct";
    modes[0].value = 0;
    modes[0].flags = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    modes[0].color_mode = MODE_COLORS_PER_LED;
    modes[0].speed_min = 0;
    modes[0].speed_max = 100;
    modes[0].colors_min = 0;
    modes[0].colors_max = num_leds;
    modes[0].brightness_min = 0;
    modes[0].brightness_max = 100;
    modes[0].brightness = 100; // Start at full brightness
    
    // Default to direct mode
    active_mode = 0;
    
    // Initialize
    SetupZones();
    Connect();
}

DDPLightDevice::~DDPLightDevice()
{
    Disconnect();
}

void DDPLightDevice::SetupZones()
{
    zones.resize(1);
    zones[0].name = "LED Strip";
    zones[0].type = ZONE_TYPE_LINEAR;
    zones[0].leds_min = num_leds;
    zones[0].leds_max = num_leds;
    zones[0].leds_count = num_leds;
    zones[0].matrix_map = nullptr;
    
    leds.resize(num_leds);
    colors.resize(num_leds);
    
    for (int i = 0; i < num_leds; i++)
    {
        leds[i].name = "LED " + std::to_string(i);
    }
    
    SetupColors();
}

void DDPLightDevice::ResizeZone(int /*zone*/, int /*new_size*/)
{
    // Not supported - fixed size
}

void DDPLightDevice::DeviceUpdateLEDs()
{
    if (!is_connected)
    {
        if (!Connect())
        {
            return;
        }
    }
    
    // Apply brightness
    std::vector<RGBColor> adjusted_colors = colors;
    unsigned int brightness = modes[active_mode].brightness;
    
    if (brightness < 100)
    {
        float brightness_factor = brightness / 100.0f;
        
        for (size_t i = 0; i < adjusted_colors.size(); i++)
        {
            unsigned char r = static_cast<unsigned char>(RGBGetRValue(adjusted_colors[i]) * brightness_factor);
            unsigned char g = static_cast<unsigned char>(RGBGetGValue(adjusted_colors[i]) * brightness_factor);
            unsigned char b = static_cast<unsigned char>(RGBGetBValue(adjusted_colors[i]) * brightness_factor);
            
            adjusted_colors[i] = ToRGBColor(r, g, b);
        }
    }
    
    // Send to device
    controller->sendRGBData(0, adjusted_colors);
}

void DDPLightDevice::UpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void DDPLightDevice::UpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void DDPLightDevice::SetCustomMode()
{
    active_mode = 0;
}

void DDPLightDevice::DeviceUpdateMode()
{
    // Only direct mode supported
    DeviceUpdateLEDs();
}

void DDPLightDevice::UpdateMode()
{
    DeviceUpdateMode();
}

bool DDPLightDevice::Connect()
{
    if (is_connected) return true;
    
    is_connected = controller->connect();
    return is_connected;
}

void DDPLightDevice::Disconnect()
{
    if (!is_connected) return;
    
    controller->disconnect();
    is_connected = false;
}

bool DDPLightDevice::IsConnected() const
{
    return is_connected;
}

QString DDPLightDevice::GetIPAddress() const
{
    return ip_address;
}