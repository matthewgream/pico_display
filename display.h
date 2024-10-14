
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

// ------------------------------------------------------------------------------------------------------------------------

#include "pico/stdlib.h"

#include "iodevice.h"

// ------------------------------------------------------------------------------------------------------------------------

#define DISPLAY_SCREEN_REFRESH      100

#define DISPLAY_SCREEN_WIDTH        DEV_EPD2IN9V2_HEIGHT
#define DISPLAY_SCREEN_HEIGHT       DEV_EPD2IN9V2_WIDTH
#define DISPLAY_SCREEN_BYTES        DEV_EPD2IN9V2_BYTES

#define DISPLAY_COLOUR_WHITE        DEV_EPD2IN9V2_WHITE
#define DISPLAY_COLOUR_BLACK        DEV_EPD2IN9V2_BLACK

#define DISPLAY_COLOUR_BACKGROUND   DISPLAY_COLOUR_BLACK
#define DISPLAY_COLOUR_FOREGROUND   DISPLAY_COLOUR_WHITE

// ------------------------------------------------------------------------------------------------------------------------

typedef struct {
    int x, y;
} position_t;

typedef int sidekey_t;

typedef struct {
    const uint8_t * data;
    int width, height;
    char base;
} fontinfo_t;

// ------------------------------------------------------------------------------------------------------------------------

extern const fontinfo_t font_noto_mono40x56;
extern const fontinfo_t font_noto_mono23x37;
extern const fontinfo_t font_noto_mono6x10;
extern const fontinfo_t font_noto_vari20x28;
extern const fontinfo_t font_icon_40x60;

// ------------------------------------------------------------------------------------------------------------------------

int display_char (const fontinfo_t * const font, const char c, const int x, const int y, const bool mono);
int display_char_sizeof (const fontinfo_t * const font, const char c, const bool mono);
int display_string (const fontinfo_t * const font, const char * const string, const int x, const int y, const int x_max, const bool mono, const int space, const bool fill);

void display_clear (const position_t corners [2]);
void display_update (void);
void display_reset (void);
void display_open (void);
void display_close (void);

// ------------------------------------------------------------------------------------------------------------------------

bool display_obtain__touched (position_t * const position);
bool display_obtain__sidekey (sidekey_t * const sidekey);

// ------------------------------------------------------------------------------------------------------------------------

#endif /*_DISPLAY_H_*/

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

