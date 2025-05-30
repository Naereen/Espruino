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
FIXME: this function is absent from the hardware, we try to simulate its behavior by a SVC_... call. See https://github.com/numworks/epsilon/issues/2326
*/
bool jswrap_battery_is_charging(void) {
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
FIXME: this function is absent from the hardware, we try to simulate its behavior by a SVC_... call. See https://github.com/numworks/epsilon/issues/2326{
*/
uint8_t jswrap_battery_level(void) {
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
FIXME: this function is absent from the hardware, we try to simulate its behavior by a SVC_... call. See https://github.com/numworks/epsilon/issues/2326
*/
float jswrap_battery_voltage(void) {
    // return eadk_battery_voltage();
    // return 0.0f;
    SVC_RETURNING_R0(SVC_BATTERY_VOLTAGE, float)
}

//
// Display
//

// FIXME: this jswrap_display_draw_string() function works but is buggued, I need to fix it!
// See: https://github.com/Naereen/A-JavaScript-interpreter-for-the-NumWorks-calculator/issues/1#issuecomment-2910837059

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
void jswrap_display_draw_string(JsVar *args) {
    int argc = jsvGetArrayLength(args);
    // Basic argument count validation
    if (argc < 6) { // Expecting at least 6 arguments
        return;
    }
    // You might also want to check for too many arguments depending on strictness

    // 2. Get 'x' (JsVar -> uint16_t)
    JsVar *xJsVar = jsvGetArrayItem(args, 1);
    if (!jsvIsInt(xJsVar)) {
        jsError("Eadk.display_draw_string: Arg[1] (x) must be integer.");
        return;
    }
    uint16_t x = (uint16_t)jsvGetInteger(xJsVar);

    // 3. Get 'y' (JsVar -> uint16_t)
    JsVar *yJsVar = jsvGetArrayItem(args, 2);
    if (!jsvIsInt(yJsVar)) {
        jsError("Eadk.display_draw_string: Arg[2] (y) must be integer.");
        return;
    }
    uint16_t y = (uint16_t)jsvGetInteger(yJsVar);

    // 4. Get 'large_font' (JsVar -> bool)
    JsVar *large_fontJsVar = jsvGetArrayItem(args, 3);
    if (!jsvIsInt(large_fontJsVar)) { // Or jsvIsInt if bools are treated as ints (0/1)
        jsError("Eadk.display_draw_string: Arg[3] (large_font) must be integer.");
        return;
    }
    bool large_font = jsvGetBool(large_fontJsVar);

    // 5. Get 'text_color' (JsVar -> uint16_t - assuming eadk_color_t is uint16_t)
    JsVar *text_colorJsVar = jsvGetArrayItem(args, 4);
    if (!jsvIsInt(text_colorJsVar)) {
        jsError("Eadk.display_draw_string: Arg[4] (text_color) must be integer.");
        return;
    }
    uint16_t text_color = (uint16_t)jsvGetInteger(text_colorJsVar);

    // 6. Get 'background_color' (JsVar -> uint16_t - assuming eadk_color_t is uint16_t)
    JsVar *background_colorJsVar = jsvGetArrayItem(args, 5);
    if (!jsvIsInt(background_colorJsVar)) {
        jsError("Eadk.display_draw_string: Arg[5] (background_color) must be integer.");
        return;
    }
    uint16_t background_color = (uint16_t)jsvGetInteger(background_colorJsVar);

    // 1. Get 'text' (JsVar -> const char*)
    JsVar *textJsVar = jsvGetArrayItem(args, 0);
    if (!jsvIsString(textJsVar)) {
        printf("Eadk.display_draw_string: Arg[0] (text) must be a string.");
        return;
    }
    // FIXED: this handling of the string was bugged
    // const char* text = jsvAsString(textJsVar); // Lock the string, so it is not garbage collected

    // We will copy the string into a local buffer, to ensure it is not garbage collected
    // and to ensure we can handle it properly, as the eadk_display_draw_string expects a C string
    size_t max_len = 31; // Updated maximum length to 31 characters for the text
    if (large_font == false) {
        max_len = 63; // If small font, we double the maximum length
    }
    char text[max_len + 1]; // Local buffer to hold the string, size max_len + 1 to allow for null-termination
    // We will use jsvGetStringChars to copy the string into our local buffer
    size_t len_of_text = jsvGetStringChars(textJsVar, 0, text, max_len);
    text[len_of_text] = '\0'; // Ensure the string is null-terminated
    // printf("Eadk.display_draw_string: text='%s'\n", text);
    if (len_of_text >= max_len) {
        printf("Eadk.display_draw_string: Arg[0] too long, %i >= max %i characters.", len_of_text, max_len);
        return;
    }

    // Call the eadk function
    eadk_display_draw_string(text, (eadk_point_t){x, y}, large_font, (eadk_color_t)text_color, (eadk_color_t)background_color);

    return;
}


// Now, we define the `jswrap_display_push_rect_uniform` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "display_push_rect_uniform",
  "generate" : "jswrap_display_push_rect_uniform",
  "params": [
    ["args", "JsVarArray", "x, y, width, height, color"]
  ]
}
Fills a rectangle on the display with a uniform color.
This can also be used to draw a single pixel by setting width and height to 1.

    x: X coordinate of the top-left corner
    y: Y coordinate of the top-left corner
    width: Width of the rectangle
    height: Height of the rectangle
    color: Color of the rectangle (e.g., 0xFFFF for white, 0xF800 for red)

For instance:
Eadk.display_push_rect_uniform(0, 0, 10, 20, 0xF800); // Fills a 10x20 red rectangle at (0,0)
Eadk.display_push_rect_uniform(50, 50, 1, 1, 0x07E0); // Draws a single green pixel at (50,50)
*/
void jswrap_display_push_rect_uniform(JsVar *args) {
    int argc = jsvGetArrayLength(args);

    // Basic argument count validation
    if (argc < 5) { // Expecting 5 arguments: x, y, width, height, color
        jsError("Eadk.display_fill_rect: Too few arguments (%i). Expected 5.", argc);
        return;
    }

    // 1. Get 'x' (JsVar -> uint16_t)
    JsVar *xJsVar = jsvGetArrayItem(args, 0);
    uint16_t x = (uint16_t) 1;
    if (!jsvIsInt(xJsVar)) {
        printf("Eadk.display_fill_rect: Arg[0] (x) must be integer.");
        // return;
    } else {
        x = (uint16_t)jsvGetInteger(xJsVar);
    }

    // 2. Get 'y' (JsVar -> uint16_t)
    JsVar *yJsVar = jsvGetArrayItem(args, 1);
    uint16_t y = (uint16_t) 1;
    if (!jsvIsInt(yJsVar)) {
        printf("Eadk.display_fill_rect: Arg[1] (y) must be integer.");
        // return;
    } else {
        y = (uint16_t)jsvGetInteger(yJsVar);
    }

    // 3. Get 'width' (JsVar -> uint16_t)
    JsVar *widthJsVar = jsvGetArrayItem(args, 2);
    uint16_t width = (uint16_t) 1;
    if (!jsvIsInt(widthJsVar)) {
        printf("Eadk.display_fill_rect: Arg[2] (width) must be integer.");
        // return;
    } else {
        width = (uint16_t)jsvGetInteger(widthJsVar);
    }

    // 4. Get 'height' (JsVar -> uint16_t)
    JsVar *heightJsVar = jsvGetArrayItem(args, 3);
    uint16_t height = (uint16_t) 1;
    if (!jsvIsInt(heightJsVar)) {
        printf("Eadk.display_fill_rect: Arg[3] (height) must be integer.");
        // return;
    } else {
        height = (uint16_t)jsvGetInteger(heightJsVar);
    }

    // 5. Get 'color' (JsVar -> uint16_t - assuming eadk_color_t is uint16_t)
    JsVar *colorJsVar = jsvGetArrayItem(args, 4);
    uint16_t color = (uint16_t) eadk_color_black; // Default to black if not provided
    if (!jsvIsInt(colorJsVar)) {
        printf("Eadk.display_fill_rect: Arg[4] (color) must be integer.\n");
        // return;
    } else {
        // If the color is provided, we convert it to uint16_t
        color = (uint16_t)jsvGetInteger(colorJsVar);
    }

    // Construct the eadk_rect_t struct
    eadk_rect_t rect = { .x = x, .y = y, .width = width, .height = height };

    // Call the eadk function
    eadk_display_push_rect_uniform(rect, (eadk_color_t)color);

    // Return from void function (as per previous success with draw_string)
    return;
}

// TODO: write these
// void jswrap_display_push_rect(JsVar *args);
// void jswrap_display_pull_rect(JsVar *args);


// Now, we define the `jswrap_display_wait_for_vblank` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "display_wait_for_vblank",
  "generate" : "jswrap_display_wait_for_vblank",
  "return" : ["bool", "Display: wait for vblank? It is not documented in eadk.h so not documented here (we don't really know what it does)"]
}
Display: wait for vblank? It is not documented in eadk.h so not documented here (we don't really know what it does)
*/
bool jswrap_display_wait_for_vblank(void) {
    return eadk_display_wait_for_vblank();
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
  "return" : ["int", "Time since boot of the machine, in milliseconds?"]
}
Time since boot of the machine, in milliseconds?
*/
uint64_t jswrap_timing_millis(void) {
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
FIXME: this function is absent from the hardware, we try to simulate its behavior by a SVC_... call. See https://github.com/numworks/epsilon/issues/2326
*/
bool jswrap_usb_is_plugged(void) {
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
uint32_t jswrap_random(void) {
    return (uint32_t) eadk_random();
}
