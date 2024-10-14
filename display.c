
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#include "pico/stdlib.h"

#include "hardware.h"
#include "display.h"

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

static const position_t __display_screen_corners [2] = { { 0, 0 }, { DISPLAY_SCREEN_WIDTH, DISPLAY_SCREEN_HEIGHT } };

static uint8_t __display_screen_buffer_data [DISPLAY_SCREEN_BYTES];

static const struct {
    uint8_t * data;
    int width;
    int height;
} __display_screen_buffer = {
    __display_screen_buffer_data,
    DEV_EPD2IN9V2_HEIGHT, DEV_EPD2IN9V2_WIDTH
};

static inline void display_pixel (const int x, const int y, const int color) {

    if (x <= __display_screen_buffer.width && y <= __display_screen_buffer.height) {

        const int addr = ((__display_screen_buffer.height - y - 1) / 8) + (x * ((__display_screen_buffer.height / 8) + (__display_screen_buffer.height % 8 == 0 ? 0 : 1)));
        const uint8_t mask = 0x80 >> ((__display_screen_buffer.height - y - 1) % 8);

        __display_screen_buffer.data [addr] = (color == DEV_EPD2IN9V2_BLACK) ? __display_screen_buffer.data [addr] & ~mask : __display_screen_buffer.data [addr] | mask;
    }
}

// ------------------------------------------------------------------------------------------------------------------------

int display_char (const fontinfo_t * const font, const char c, const int x, const int y, const bool mono) {

    const int font_rows = (font->height / 8) + (font->height % 8 ? 1 : 0);
    const int char_offs = ((font->width * font_rows) + 1) * (c - font->base);
    const int char_width = mono ? font->width : (int) font->data [char_offs];

    for (int i = 0; i < char_width; i ++) {

        for (int j = 0; j < font_rows; j ++) {

            const uint8_t char_byte = font->data [(char_offs + 1) + (i * font_rows) + j];

            for (int k = 0; k < 8; k ++)
                display_pixel (x + i, y + (j * 8) + k, (char_byte & (1 << k)) ? DISPLAY_COLOUR_FOREGROUND : DISPLAY_COLOUR_BACKGROUND);
        }
    }

    return char_width;
}

int display_char_sizeof (const fontinfo_t * const font, const char c, const bool mono) {

    const int font_rows = (font->height / 8) + (font->height % 8 ? 1 : 0);
    const int char_offs = ((font->width * font_rows) + 1) * (c - font->base);

    return mono ? font->width : (int) font->data [char_offs];
}

int display_string (const fontinfo_t * const font, const char * const string, const int x, const int y, const int x_max, const bool mono, const int space, const bool fill) {

    const char * ptr = string;
    int xp = x;

    while (*ptr != '\0') {
        const int w = display_char_sizeof (font, *ptr, mono);
        if (!(xp + w < x_max))
            break;
        display_char (font, *ptr ++, xp, y, mono);
        xp += w + space;
    }

    if (fill) {
        int xq = xp;
        while (xq < x_max) {
            for (int yq = 0; yq < font->height; yq ++)
                display_pixel (xq, y + yq, DISPLAY_COLOUR_BACKGROUND);
            xq ++;
        }
    }

    return (xp - x);
}

// ------------------------------------------------------------------------------------------------------------------------

void display_clear (const position_t corners [2]) {

    for (int y = corners [0].y; y < corners [1].y; y++)
        for (int x = corners [0].x; x < corners [1].x; x++)
            display_pixel (x, y, DISPLAY_COLOUR_BACKGROUND);
}

void display_update (void) {

    static int display_updates = 0;
    DEV_display (__display_screen_buffer_data, sizeof (__display_screen_buffer_data), (display_updates ++ % DISPLAY_SCREEN_REFRESH) == 0);
}

void display_reset (void) {

    display_clear (__display_screen_corners);
}

// ------------------------------------------------------------------------------------------------------------------------

void display_open (void) {

    DEV_init ();
    display_reset ();
}

void display_close (void) {

    DEV_exit ();
}

// ------------------------------------------------------------------------------------------------------------------------

bool display_obtain__touched (position_t * const position) {
    int p;
    if (!DEV_touch (&position->x, &position->y, &p))
        return false;
    /* filter on low p XXX */
    return true;
}

bool display_obtain__sidekey (sidekey_t * const sidekey) {

    return DEV_key (sidekey);
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

