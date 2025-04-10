#pragma once

#include <string>
#include <vector>

/*-----------------------------------------*\
| RGB color type definitions                |
\*-----------------------------------------*/
typedef unsigned int RGBColor;

#define RGBGetRValue(rgb)    ((rgb >> 16) & 0xFF)
#define RGBGetGValue(rgb)    ((rgb >> 8) & 0xFF)
#define RGBGetBValue(rgb)    (rgb & 0xFF)

#define ToRGBColor(r,g,b)    ((r << 16) | (g << 8) | b)

/*-----------------------------------------*\
| Device type definitions                   |
\*-----------------------------------------*/
enum device_type
{
    DEVICE_TYPE_MOTHERBOARD     = 0,
    DEVICE_TYPE_DRAM            = 1,
    DEVICE_TYPE_GPU             = 2,
    DEVICE_TYPE_COOLER          = 3,
    DEVICE_TYPE_LEDSTRIP        = 4,
    DEVICE_TYPE_KEYBOARD        = 5,
    DEVICE_TYPE_MOUSE           = 6,
    DEVICE_TYPE_MOUSEMAT        = 7,
    DEVICE_TYPE_HEADSET         = 8,
    DEVICE_TYPE_HEADSET_STAND   = 9,
    DEVICE_TYPE_UNKNOWN         = 10
};

/*-----------------------------------------*\
| Zone type definitions                     |
\*-----------------------------------------*/
enum zone_type
{
    ZONE_TYPE_SINGLE            = 0,
    ZONE_TYPE_LINEAR            = 1,
    ZONE_TYPE_MATRIX            = 2
};

/*-----------------------------------------*\
| Mode flag and color mode definitions      |
\*-----------------------------------------*/
#define MODE_FLAG_HAS_SPEED           (1 << 0)
#define MODE_FLAG_HAS_DIRECTION_LR    (1 << 1)
#define MODE_FLAG_HAS_DIRECTION_UD    (1 << 2)
#define MODE_FLAG_HAS_DIRECTION_HV    (1 << 3)
#define MODE_FLAG_HAS_BRIGHTNESS      (1 << 4)
#define MODE_FLAG_HAS_PER_LED_COLOR   (1 << 5)
#define MODE_FLAG_HAS_MODE_SPECIFIC_COLOR (1 << 6)
#define MODE_FLAG_HAS_RANDOM_COLOR    (1 << 7)

enum color_mode
{
    MODE_COLORS_NONE            = 0,
    MODE_COLORS_PER_LED         = 1,
    MODE_COLORS_MODE_SPECIFIC   = 2,
    MODE_COLORS_RANDOM          = 3
};

/*-----------------------------------------*\
| Mode direction definitions                |
\*-----------------------------------------*/
enum mode_direction
{
    MODE_DIRECTION_LEFT         = 0,
    MODE_DIRECTION_RIGHT        = 1,
    MODE_DIRECTION_UP           = 2,
    MODE_DIRECTION_DOWN         = 3,
    MODE_DIRECTION_HORIZONTAL   = 4,
    MODE_DIRECTION_VERTICAL     = 5
};

/*-----------------------------------------*\
| Mode struct definition                    |
\*-----------------------------------------*/
struct mode
{
    std::string                 name;
    int                         value;
    unsigned int                flags;
    unsigned int                speed_min;
    unsigned int                speed_max;
    unsigned int                colors_min;
    unsigned int                colors_max;
    unsigned int                speed;
    unsigned int                direction;
    unsigned int                color_mode;
    std::vector<RGBColor>       colors;

    mode() :
        value(0),
        flags(0),
        speed_min(0),
        speed_max(0),
        colors_min(0),
        colors_max(0),
        speed(0),
        direction(0),
        color_mode(MODE_COLORS_NONE)
    { }
};

/*-----------------------------------------*\
| Zone struct definition                    |
\*-----------------------------------------*/
struct zone
{
    std::string                 name;
    zone_type                   type;
    unsigned int                leds_min;
    unsigned int                leds_max;
    unsigned int                leds_count;
    unsigned int                start_idx;
    unsigned int*               matrix_map;

    zone() :
        type(ZONE_TYPE_SINGLE),
        leds_min(0),
        leds_max(0),
        leds_count(0),
        start_idx(0),
        matrix_map(nullptr)
    { }
};

/*-----------------------------------------*\
| LED struct definition                     |
\*-----------------------------------------*/
struct led
{
    std::string                 name;
    unsigned int                value;

    led() :
        value(0)
    { }
};