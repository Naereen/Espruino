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

#include "jswrap_eadk.h"  // We need the declaration of the jswrap_... function

#include "eadk.h" // For all the NumWorks's EADK functions


// Let's define the JavaScript class that will contain all our `eadk_...` methods and attributes. We'll call it `Eadk`
/*JSON{
  "type" : "class",
  "class" : "Eadk"
}
A Eadk class to expose some functions, made available to the C programming environment from eadk.h (from [NumWorks C API](https://github.com/numworks/epsilon/blob/master/eadk/include/eadk/eadk.h)).
*/

//
// Backlight brightness
//

// Now, we define the `jswrap_backlight_brightness` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "backlight_brightness",
  "generate" : "jswrap_backlight_brightness",
  "return": ["int", "Backlight brightness"]
}*/
// uint8_t
int
// JsVarInt
jswrap_backlight_brightness(void) {
    // TODO: check if the jsiConsolePrint() and jsiConsolePrintf() both work correctly.
    jsiConsolePrint("Call to jswrap_backlight_brightness()!\r\n");
    int result = (int) eadk_backlight_brightness(); // call, then convert from uint8_t to int
    jsiConsolePrintf("Got backlight brightness = %i\r\n", result);
    return result;
}

// Now, we define the `jswrap_set_backlight_brightness` to be a `staticmethod` on the `Eadk` class
/*JSON{
  "type" : "staticmethod",
  "class" : "Eadk",
  "name" : "set_backlight_brightness",
  "generate" : "jswrap_set_backlight_brightness",
  "params": [
    ["brightness", "int", "Backlight brightness"]
  ]
}*/
void jswrap_set_backlight_brightness(int brightness) {
    // TODO: check if the jsiConsolePrint() and jsiConsolePrintf() both work correctly.
    jsiConsolePrint("Call to jswrap_set_backlight_brightness()!\r\n");
    eadk_backlight_set_brightness(brightness);
}

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
