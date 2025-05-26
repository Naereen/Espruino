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
int jswrap_color_black(void) {
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
int jswrap_color_white(void) {
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
int jswrap_color_red(void) {
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
int jswrap_color_green(void) {
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
int jswrap_color_blue(void) {
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
  "return" : ["int", "The screen width (320 pixels)"]
}
The screen width (320 pixels)
*/
int jswrap_SCREEN_WIDTH(void) {
    return 320;
}

/*JSON{
  "type" : "staticproperty",
  "class" : "Eadk",
  "name" : "SCREEN_HEIGHT",
  "generate" : "jswrap_SCREEN_HEIGHT",
  "return" : ["int", "The screen height (240 pixels)"]
}
The screen height (240 pixels).
*/
int jswrap_SCREEN_HEIGHT(void) {
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
    return eadk_battery_is_charging();
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
    return eadk_battery_level();
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
    return eadk_battery_voltage();
}

//
// Display
//

// TODO: find a way to have a function accept a `char*` parameter

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
    ["us", "int", "Duration of sleep, in micro-seconds"]
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
    ["ms", "int", "Duration of sleep, in milli-seconds"]
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
    return eadk_usb_is_plugged();
}


// Now, we define the `jswrap_random` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "random",
  "generate" : "jswrap_random",
  "return" : ["int", "An almost truly random number, generated from the hardware RNG (a uint32_t)"]
}
An almost truly random number, generated from the hardware RNG (a uint32_t)
*/
uint32_t jswrap_random() {
    return eadk_random();
}
