
#ifndef _IODEVICE_H_
#define _IODEVICE_H_

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

#include "pico/stdlib.h"

// ------------------------------------------------------------------------------------------------

#define DEV_EPD2IN9V2_BLACK      0x00
#define DEV_EPD2IN9V2_WHITE      0xFF

#define DEV_EPD2IN9V2_WIDTH      128
#define DEV_EPD2IN9V2_HEIGHT     296
#define DEV_EPD2IN9V2_BYTES      (((DEV_EPD2IN9V2_WIDTH % 8 == 0) ? (DEV_EPD2IN9V2_WIDTH / 8) : (DEV_EPD2IN9V2_WIDTH / 8 + 1)) * DEV_EPD2IN9V2_HEIGHT)

// ------------------------------------------------------------------------------------------------

void DEV_init (void);
void DEV_exit (void);

void DEV_display (const uint8_t * const image, const int length, const bool full);
bool DEV_key (int * const key_value);
bool DEV_touch (int * const touch_x, int * const touch_y, int * const touch_p);

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

#endif

