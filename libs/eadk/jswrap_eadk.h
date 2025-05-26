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

// Backlight

int jswrap_backlight_brightness(void);
void jswrap_set_backlight_brightness(int brightness);

// Colors

int jswrap_color_black(void);
int jswrap_color_white(void);
int jswrap_color_red(void);
int jswrap_color_green(void);
int jswrap_color_blue(void);