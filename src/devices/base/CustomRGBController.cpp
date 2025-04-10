#include "CustomRGBController.h"

/*-----------------------------------------*\
| CustomRGBController implementation        |
\*-----------------------------------------*/

CustomRGBController::CustomRGBController() :
    active_mode(0)
{
    // Constructor implementation with initialization of active_mode
}

CustomRGBController::~CustomRGBController()
{
    // Clean up any allocated resources
    for(std::size_t zone_idx = 0; zone_idx < zones.size(); zone_idx++)
    {
        if(zones[zone_idx].matrix_map != nullptr)
        {
            delete[] zones[zone_idx].matrix_map;
            zones[zone_idx].matrix_map = nullptr;
        }
    }
}

void CustomRGBController::SetLED(int led, RGBColor color)
{
    if(led >= 0 && led < static_cast<int>(colors.size()))
    {
        colors[led] = color;
    }
}

void CustomRGBController::SetAllLEDs(RGBColor color)
{
    for(std::size_t led = 0; led < colors.size(); led++)
    {
        colors[led] = color;
    }
}

void CustomRGBController::SetAllZoneLEDs(int zone, RGBColor color)
{
    if(zone >= 0 && zone < static_cast<int>(zones.size()))
    {
        for(std::size_t led_idx = 0; led_idx < zones[zone].leds_count; led_idx++)
        {
            SetLED(static_cast<int>(zones[zone].start_idx + led_idx), color);
        }
    }
}