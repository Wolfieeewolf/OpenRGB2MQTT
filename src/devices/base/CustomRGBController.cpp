#include "CustomRGBController.h"

CustomRGBController::CustomRGBController()
{
}

CustomRGBController::~CustomRGBController()
{
}

void CustomRGBController::SetLED(int led, RGBColor color)
{
    if(led < static_cast<int>(colors.size()))
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
    for(std::size_t led_idx = 0; led_idx < zones[zone].leds_count; led_idx++)
    {
        SetLED(static_cast<int>(zones[zone].start_idx + led_idx), color);
    }
}