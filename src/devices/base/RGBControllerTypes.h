#pragma once

#include <string>

typedef unsigned int RGBColor;

#define RGBGetRValue(rgb)    ((rgb >> 16) & 0xFF)
#define RGBGetGValue(rgb)    ((rgb >> 8) & 0xFF)
#define RGBGetBValue(rgb)    (rgb & 0xFF)

#define ToRGBColor(r,g,b)    ((r << 16) | (g << 8) | b)

enum device_type
{
    DEVICE_TYPE_LEDSTRIP,
    DEVICE_TYPE_UNKNOWN
};

enum zone_type
{
    ZONE_TYPE_SINGLE
};

#define MODE_FLAG_HAS_PER_LED_COLOR  0x01

enum color_mode
{
    MODE_COLORS_PER_LED
};

struct mode
{
    std::string name;
    int value;
    unsigned int flags;
    unsigned int color_mode;
    mode() : value(0), flags(0), color_mode(MODE_COLORS_PER_LED) {}
};

struct zone
{
    std::string name;
    zone_type type;
    unsigned int leds_min;
    unsigned int leds_max;
    unsigned int leds_count;
    unsigned int start_idx;
    unsigned int* matrix_map;
    zone() : type(ZONE_TYPE_SINGLE), leds_min(0), leds_max(0), leds_count(0), start_idx(0), matrix_map(nullptr) {}
};

struct led
{
    std::string name;
    led() {}
};