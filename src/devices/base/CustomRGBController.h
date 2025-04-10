#pragma once

#include <string>
#include <vector>
#include "RGBControllerTypes.h"

/*-----------------------------------------*\
| CustomRGBController API                   |
| This is an intermediate class that allows |
| for better compatibility with OpenRGB API |
\*-----------------------------------------*/

class CustomRGBController
{
public:
    CustomRGBController();
    virtual ~CustomRGBController();

    /*-----------------------------------------*\
    | Device information fields                 |
    \*-----------------------------------------*/
    std::string                     name;
    std::string                     vendor;
    std::string                     description;
    std::string                     version;
    std::string                     serial;
    std::string                     location;
    device_type                     type;

    /*-----------------------------------------*\
    | Device structure fields                   |
    \*-----------------------------------------*/
    std::vector<mode>              modes;
    std::vector<zone>              zones;
    std::vector<led>               leds;
    std::vector<RGBColor>          colors;
    unsigned int                   active_mode;

    /*-----------------------------------------*\
    | Required virtual functions                |
    \*-----------------------------------------*/
    virtual void                    SetupZones() = 0;
    virtual void                    ResizeZone(int zone, int new_size) = 0;
    virtual void                    DeviceUpdateLEDs() = 0;
    virtual void                    UpdateZoneLEDs(int zone) = 0;
    virtual void                    UpdateSingleLED(int led) = 0;
    virtual void                    SetCustomMode() = 0;
    virtual void                    DeviceUpdateMode() = 0;

    /*-----------------------------------------*\
    | Helper functions                          |
    \*-----------------------------------------*/
    void                            SetLED(int led, RGBColor color);
    void                            SetAllLEDs(RGBColor color);
    void                            SetAllZoneLEDs(int zone, RGBColor color);
};