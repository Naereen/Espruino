//
// A JavaScript jswrap_eadk library around the eadk.h library offered by the NumWorks environment.
//
// See https://github.com/Naereen/A-JavaScript-interpreter-for-the-NumWorks-calculator/issues/2
//
// Documentation on the eadk.h library? Its source:
// See https://github.com/numworks/epsilon/blob/master/eadk/include/eadk/eadk.h
//
// Documentation on how to complete this file?
// See https://github.com/espruino/Espruino/blob/master/libs/README.md
//

#include "jsvar.h" // For the JsVar* types
#include "jsinteractive.h" // For the jsiConsolePrint function

#include "jswrap_eadk.h"  // We need the declaration of the jswrap_... function

#include "eadk.h" // For all the NumWorks's EADK functions
#include "svcall.h" // The SVCall.h library taken from NumWorks

// #include <stdint.h>
#include <stdbool.h>


// Let's define the JavaScript class that will contain all our `eadk_...` methods and attributes. We'll call it `Eadk`
/*JSON{
  "type" : "class",
  "class" : "Eadk"
}
A class named 'Eadk', to expose some functions, made available to the C programming environment from eadk.h (from [NumWorks C API](https://github.com/numworks/epsilon/blob/master/eadk/include/eadk/eadk.h)).
*/

//
// Colors
//

/*JSON{
  "type" : "staticproperty",
  "class" : "Eadk",
  "name" : "color_black",
  "generate" : "jswrap_color_black",
  "return" : ["int", "The black color"]
}
The black color.
*/
int16_t jswrap_color_black(void) {
    return 0x0;
}

/*JSON{
  "type" : "staticproperty",
  "class" : "Eadk",
  "name" : "color_white",
  "generate" : "jswrap_color_white",
  "return" : ["int", "The white color"]
}
The white color.
*/
int16_t jswrap_color_white(void) {
    return 0xFFFF;
}

/*JSON{
  "type" : "staticproperty",
  "class" : "Eadk",
  "name" : "color_red",
  "generate" : "jswrap_color_red",
  "return" : ["int", "The red color"]
}
The red color.
*/
int16_t jswrap_color_red(void) {
    return 0xF800;
}

/*JSON{
  "type" : "staticproperty",
  "class" : "Eadk",
  "name" : "color_green",
  "generate" : "jswrap_color_green",
  "return" : ["int", "The green color"]
}
The green color.
*/
int16_t jswrap_color_green(void) {
    return 0x07E0;
}

/*JSON{
  "type" : "staticproperty",
  "class" : "Eadk",
  "name" : "color_blue",
  "generate" : "jswrap_color_blue",
  "return" : ["int", "The blue color"]
}
The blue color.
*/
int16_t jswrap_color_blue(void) {
    return 0x001F;
}

//
// Screen width and height
//

/*JSON{
  "type" : "staticproperty",
  "class" : "Eadk",
  "name" : "SCREEN_WIDTH",
  "generate" : "jswrap_SCREEN_WIDTH",
  "return" : ["int32", "The screen width (320 pixels)"]
}
The screen width (320 pixels)
*/
int32_t jswrap_SCREEN_WIDTH(void) {
    return 320;
}

/*JSON{
  "type" : "staticproperty",
  "class" : "Eadk",
  "name" : "SCREEN_HEIGHT",
  "generate" : "jswrap_SCREEN_HEIGHT",
  "return" : ["int32", "The screen height (240 pixels)"]
}
The screen height (240 pixels).
*/
int32_t jswrap_SCREEN_HEIGHT(void) {
    return 240;
}

//
// Backlight brightness
//

// Now, we define the `jswrap_backlight_brightness` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "backlight_brightness",
  "generate" : "jswrap_backlight_brightness",
  "return" : ["int", "Backlight brightness"]
}
Backlight brightness
*/
int jswrap_backlight_brightness(void) {
    int result = (int) eadk_backlight_brightness(); // call, then convert from uint8_t to int
    return result;
}

// Now, we define the `jswrap_backlight_set_brightness` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "backlight_set_brightness",
  "generate" : "jswrap_backlight_set_brightness",
  "params": [
    ["brightness", "int", "Set backlight brightness"]
  ]
}
Set backlight brightness
*/
void jswrap_backlight_set_brightness(int brightness) {
    eadk_backlight_set_brightness(brightness);
}

//
// Battery
//

// Now, we define the `jswrap_battery_is_charging` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "battery_is_charging",
  "generate" : "jswrap_battery_is_charging",
  "return" : ["bool", "Battery is charging?"]
}
Battery is charging?
*/
bool jswrap_battery_is_charging(void) {
    // FIXME: See https://github.com/numworks/epsilon/issues/2326
    // return eadk_battery_is_charging();
    // return false;
    SVC_RETURNING_R0(SVC_BATTERY_IS_CHARGING, bool)
}

// Now, we define the `jswrap_battery_level` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "battery_level",
  "generate" : "jswrap_battery_level",
  "return" : ["int", "Battery level (a uint8_t integer)"]
}
Battery level (a uint8_t integer)
*/
uint8_t jswrap_battery_level(void) {
    // FIXME: See https://github.com/numworks/epsilon/issues/2326{
    // return eadk_battery_level();
    // return 0;
    SVC_RETURNING_R0(SVC_BATTERY_LEVEL, uint8_t)
}

// Now, we define the `jswrap_battery_voltage` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "battery_voltage",
  "generate" : "jswrap_battery_voltage",
  "return" : ["float", "Battery voltage (a float value)"]
}
Battery voltage (a float value)
*/
float jswrap_battery_voltage(void) {
    // FIXME: See https://github.com/numworks/epsilon/issues/2326
    // return eadk_battery_voltage();
    // return 0.0f;
    SVC_RETURNING_R0(SVC_BATTERY_VOLTAGE, float)
}

//
// Display
//

// TODO: find a way to have a function accept a `char*` parameter

// Now, we define the `jswrap_display_draw_string` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "display_draw_string",
  "generate" : "jswrap_display_draw_string",
  "params" : [
    ["args", "JsVarArray", "Text, x, y, large_font, text_color, background_color"]
  ]
}
Display a given text string, at a {x,y} position, in large/small font, with text_color and background_color.

For instance:
Eadk.display_draw_string(text, x, y, large, text_color, background_color);
*/
// The C function signature must now be the standard JsVarArray one
void jswrap_display_draw_string(JsVar *args) {
    int argc = jsvGetArrayLength(args);
    // Basic argument count validation
    if (argc < 6) { // Expecting at least 6 arguments
        jsError("Arg error: too few args (%i)", argc);
        return;
    }
    // You might also want to check for too many arguments depending on strictness

    // 1. Get 'text' (JsVar -> const char*)
    if (!jsvIsString(jsvGetArrayItem(args, 0))) {
        jsError("Arg error: bad arg type [0]");
        return;
    }
    const char* text = jsvAsString(jsvGetArrayItem(args, 0));

    // 2. Get 'x' (JsVar -> uint16_t)
    if (!jsvIsInt(jsvGetArrayItem(args, 1))) {
        jsError("Arg error: bad arg type [1]");
        return;
    }
    uint16_t x = (uint16_t)jsvGetInteger(jsvGetArrayItem(args, 1));

    // 3. Get 'y' (JsVar -> uint16_t)
    if (!jsvIsInt(jsvGetArrayItem(args, 2))) {
        jsError("Arg error: bad arg type [2]");
        return;
    }
    uint16_t y = (uint16_t)jsvGetInteger(jsvGetArrayItem(args, 2));

    // 4. Get 'large_font' (JsVar -> bool)
    if (!jsvIsInt(jsvGetArrayItem(args, 3))) { // Or jsvIsInt if bools are treated as ints (0/1)
        jsError("Arg error: bad arg type [3]");
        return;
    }
    bool large_font = jsvGetBool(jsvGetArrayItem(args, 3));

    // 5. Get 'text_color' (JsVar -> uint16_t - assuming eadk_color_t is uint16_t)
    if (!jsvIsInt(jsvGetArrayItem(args, 4))) {
        jsError("Arg error: bad arg type [4]");
        return;
    }
    uint16_t text_color = (uint16_t)jsvGetInteger(jsvGetArrayItem(args, 4));

    // 6. Get 'background_color' (JsVar -> uint16_t - assuming eadk_color_t is uint16_t)
    if (!jsvIsInt(jsvGetArrayItem(args, 5))) {
        jsError("Arg error: bad arg type [5]");
        return;
    }
    uint16_t background_color = (uint16_t)jsvGetInteger(jsvGetArrayItem(args, 5));

    // Call the eadk function
    eadk_display_draw_string(text, (eadk_point_t){x, y}, large_font, (eadk_color_t)text_color, (eadk_color_t)background_color);

    return;
}

//
// Timing
//


// Now, we define the `jswrap_timing_usleep` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "timing_usleep",
  "generate" : "jswrap_timing_usleep",
  "params": [
    ["us", "int32", "Duration of sleep, in micro-seconds"]
  ]
}
Sleep for us micro-seconds
*/
void jswrap_timing_usleep(uint32_t us) {
    return eadk_timing_usleep(us);
}


// Now, we define the `jswrap_timing_msleep` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "timing_msleep",
  "generate" : "jswrap_timing_msleep",
  "params": [
    ["ms", "int32", "Duration of sleep, in milli-seconds"]
  ]
}
Sleep for ms milli-seconds
*/
void jswrap_timing_msleep(uint32_t ms) {
    return eadk_timing_msleep(ms);
}


// Now, we define the `jswrap_timing_millis` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "timing_millis",
  "generate" : "jswrap_timing_millis",
  "return" : ["int", "Time since boot of the machine? Not clear. FIXME:"]
}
Time since boot of the machine? Not clear. FIXME:
*/
uint64_t jswrap_timing_millis() {
    return eadk_timing_millis();
}

//
// Misc
//


// Now, we define the `jswrap_usb_is_plugged` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "usb_is_plugged",
  "generate" : "jswrap_usb_is_plugged",
  "return" : ["bool", "Is the calculator plugged on USB?"]
}
Is the calculator plugged on USB?
*/
bool jswrap_usb_is_plugged() {
    // FIXME: See https://github.com/numworks/epsilon/issues/2326
    // return eadk_usb_is_plugged();
    // return false;
    SVC_RETURNING_R0(SVC_USB_IS_PLUGGED, bool)
}


// Now, we define the `jswrap_random` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "random",
  "generate" : "jswrap_random",
  "return" : ["int32", "An almost truly random number, generated from the hardware RNG (a uint32_t)"]
}
An almost truly random number, generated from the hardware RNG (a uint32_t)
*/
uint32_t jswrap_random() {
    return eadk_random();
}
