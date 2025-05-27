//
// A JavaScript jswrap_eadk library around the eadk.h library offered by the NumWorks environment.
//
// Documentation on the eadk.h library? Its source:
// See https://github.com/numworks/epsilon/blob/master/eadk/include/eadk/eadk.h
//
// Documentation on how to complete this file?
// See https://github.com/espruino/Espruino/blob/master/libs/README.md
//

#include "jsvar.h" // For the JsVar* types
#include "jsinteractive.h" // For the jsiConsolePrint function

#include "eadk.h" // For all the NumWorks's EADK functions

#include <stdbool.h>

// Colors

int16_t jswrap_color_black(void);
int16_t jswrap_color_white(void);
int16_t jswrap_color_red(void);
int16_t jswrap_color_green(void);
int16_t jswrap_color_blue(void);

// Screen width and height

int32_t jswrap_SCREEN_WIDTH(void);
int32_t jswrap_SCREEN_HEIGHT(void);

// Backlight

int jswrap_backlight_brightness(void);
void jswrap_backlight_set_brightness(int brightness);

// Battery

bool jswrap_battery_is_charging();
uint8_t jswrap_battery_level();
float jswrap_battery_voltage();

// Display

void jswrap_display_draw_string(JsVar *args);
void jswrap_display_push_rect_uniform(JsVar *args);

// void jswrap_display_push_rect(JsVar *args);
// void jswrap_display_pull_rect(JsVar *args);
// bool jswrap_display_wait_for_vblank();

// Timing

void jswrap_timing_usleep(uint32_t us);
void jswrap_timing_msleep(uint32_t ms);
uint64_t jswrap_timing_millis();

// Misc

bool jswrap_usb_is_plugged();
uint32_t jswrap_random();
