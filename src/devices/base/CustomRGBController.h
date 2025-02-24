#pragma once

#include <string>
#include <vector>
#include "RGBControllerTypes.h"

class CustomRGBController
{
public:
    CustomRGBController();
    virtual ~CustomRGBController();

    std::string                     name;
    std::string                     vendor;
    std::string                     description;
    std::string                     version;
    std::string                     serial;
    std::string                     location;
    device_type                     type;

    std::vector<mode>              modes;
    std::vector<zone>              zones;
    std::vector<led>               leds;
    std::vector<RGBColor>          colors;

    virtual void                    SetupZones() = 0;
    virtual void                    ResizeZone(int zone, int new_size) = 0;
    virtual void                    DeviceUpdateLEDs() = 0;
    virtual void                    UpdateZoneLEDs(int zone) = 0;
    virtual void                    UpdateSingleLED(int led) = 0;
    virtual void                    SetCustomMode() = 0;
    virtual void                    DeviceUpdateMode() = 0;

    void                           SetLED(int led, RGBColor color);
    void                           SetAllLEDs(RGBColor color);
    void                           SetAllZoneLEDs(int zone, RGBColor color);
};