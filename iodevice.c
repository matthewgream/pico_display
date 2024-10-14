
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

#include "iodevice.h"

#include "hardware/spi.h"
#include "hardware/i2c.h"

// ------------------------------------------------------------------------------------------------

#define SPI_PORT            spi1

#define I2C_PORT            i2c1
#define I2C_ADDR            0x48

#define DEV_EPDDC           8
#define DEV_EPDCS           9
#define DEV_EPDCLK          10
#define DEV_EPDMOS          11
#define DEV_EPDRST          12
#define DEV_EPDBSY          13

#define DEV_GPIO_TP_SDA     6    //6,1 * 20,0
#define DEV_GPIO_TP_SCL     7    //7,1 * 21,0
#define DEV_GPIO_TP_RST     16
#define DEV_GPIO_TP_INT     17

#define DEV_GPIO_KEY0       2
#define DEV_GPIO_KEY1       3
#define DEV_GPIO_KEY2       15

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

static inline void __delay (const uint32_t ms) {
    sleep_ms (ms);
}
static inline void __delay_us (const uint64_t us) {
    sleep_us (us);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

static void __gpio_mode (const int pin, const int mode, const bool pullup) {
    gpio_init (pin);
    gpio_set_dir (pin, mode == 0 || mode == GPIO_IN ? GPIO_IN : GPIO_OUT);
    if (pullup) gpio_pull_up (pin);
}
static inline void __gpio_set (const int pin, const uint8_t value) {
    gpio_put (pin, value);
}
static inline uint8_t __gpio_get (const int pin) {
    return gpio_get (pin);
}
static void __gpio_init (void) {
    __gpio_mode (DEV_EPDRST, 1, false);
    __gpio_mode (DEV_EPDDC, 1, false);
    __gpio_mode (DEV_EPDCS, 1, false);
    __gpio_mode (DEV_EPDBSY, 0, false);
    __gpio_mode (DEV_GPIO_TP_RST, 1, false);
    __gpio_mode (DEV_GPIO_TP_INT, 0, false);
    __gpio_mode (DEV_GPIO_KEY0, 0, true);
    __gpio_mode (DEV_GPIO_KEY1, 0, true);
    __gpio_mode (DEV_GPIO_KEY2, 0, true);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

static inline void __spi_write (const uint8_t * const data, const int len) {
    spi_write_blocking (SPI_PORT, data, len);
}
static void __spi_init (void) {
    spi_init (SPI_PORT, 5 * 1000 * 1000);
    gpio_set_function (DEV_EPDCLK, GPIO_FUNC_SPI);
    gpio_set_function (DEV_EPDMOS, GPIO_FUNC_SPI);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

static inline void __i2c_write (const int reg, const uint8_t * const data, const int len) {
    uint8_t wbuf [50] = { (reg >> 8) & 0xff, reg & 0xff };
    for (int i = 0; i < len; i ++)
        wbuf [i + 2] = data [i];
    i2c_write_blocking (I2C_PORT, I2C_ADDR, wbuf, len + 2, false);
}
static inline void __i2c_read (const int reg, uint8_t * const data, const int len) {
    const uint8_t wbuf [2] = { (reg >> 8) & 0xff, reg & 0xff };
    i2c_write_blocking (I2C_PORT, I2C_ADDR, wbuf, sizeof (wbuf), false);
    i2c_read_blocking (I2C_PORT, I2C_ADDR, data, len, false);
}
static void __i2c_init (void) {
    i2c_init (I2C_PORT, 100 * 1000);
    gpio_set_function (DEV_GPIO_TP_SDA, GPIO_FUNC_I2C);
    gpio_set_function (DEV_GPIO_TP_SCL, GPIO_FUNC_I2C);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

#define __icnt8x_touch_max 5
#define __icnt8x_touch_len 7

typedef struct {
    int touch;
    int count;
    uint16_t X [__icnt8x_touch_max];
    uint16_t Y [__icnt8x_touch_max];
    uint8_t  P [__icnt8x_touch_max];
} __icnt86x_dev_t;

__icnt86x_dev_t __icnt86x_dev_now, __icnt86x_dev_old;

// ------------------------------------------------------------------------------------------------

static void __icnt86x_reset (void) {

    __gpio_set (DEV_GPIO_TP_RST, 1);
    __delay (100);
    __gpio_set (DEV_GPIO_TP_RST, 0);
    __delay (100);
    __gpio_set (DEV_GPIO_TP_RST, 1);
    __delay (100);
}

// ------------------------------------------------------------------------------------------------

static bool __icnt86x_scan (void) {

    if (!__icnt86x_dev_now.touch)
        return false;
    __icnt86x_dev_now.touch = 0;

    bool result = false;

    uint8_t buf [__icnt8x_touch_max * __icnt8x_touch_len];

    __i2c_read (0x1001, buf, 1);

    if (buf [0] > 0 && buf [0] <= __icnt8x_touch_max) {

        __icnt86x_dev_now.count = (int) buf [0];

        __i2c_read (0x1002, buf, __icnt86x_dev_now.count * __icnt8x_touch_len);

        __icnt86x_dev_old.X [0] = __icnt86x_dev_now.X [0];
        __icnt86x_dev_old.Y [0] = __icnt86x_dev_now.Y [0];
        __icnt86x_dev_old.P [0] = __icnt86x_dev_now.P [0];

        for (int i = 0; i < __icnt86x_dev_now.count; i++) {
            __icnt86x_dev_now.X [i] = ((uint16_t) buf [2 + __icnt8x_touch_len*i] << 8) + (uint16_t) buf [1 + __icnt8x_touch_len*i];
            __icnt86x_dev_now.Y [i] = ((uint16_t) buf [4 + __icnt8x_touch_len*i] << 8) + (uint16_t) buf [3 + __icnt8x_touch_len*i];
            __icnt86x_dev_now.P [i] = buf [5 + __icnt8x_touch_len*i];
        }

        result = true;
    }

    const uint8_t mask = 0x00;
    __i2c_write (0x1001, &mask, 1);
    __delay (1);

    return result;
}

// ------------------------------------------------------------------------------------------------

static void __icnt86x_intr_handler (__attribute__ ((unused)) unsigned int a, __attribute__ ((unused)) long unsigned int b) {

    __icnt86x_dev_now.touch = ! __gpio_get (DEV_GPIO_TP_INT);
}
static void __icnt86x_init (void) {

    __icnt86x_reset ();
    gpio_set_irq_enabled_with_callback (DEV_GPIO_TP_INT, 0b0100, true, __icnt86x_intr_handler);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

static const uint8_t __epd2in9v2_WF_PARTIAL_2IN9 [159] = {
    0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0A,0x00,0x00,0x00,
    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x22,0x22,0x00,0x00,0x00,0x22,0x17,0x41,0xB0,0x32,0x36,
};

static uint8_t __epd2in9v2_BLACK [DEV_EPD2IN9V2_BYTES];

static void DEV_epd2in9v2_black (void) {

    for (int i = 0; i < (int) sizeof (__epd2in9v2_BLACK); i ++)
        __epd2in9v2_BLACK [i] = DEV_EPD2IN9V2_BLACK;
}

// ------------------------------------------------------------------------------------------------

static void DEV_epd2in9v2_reset (void) {

    __gpio_set (DEV_EPDRST, 1);
    __delay (20);
    __gpio_set (DEV_EPDRST, 0);
    __delay (2);
    __gpio_set (DEV_EPDRST, 1);
    __delay (20);
}

static void DEV_epd2in9v2_send_cmnd (const uint8_t reg) {

    __gpio_set (DEV_EPDDC, 0);
    __gpio_set (DEV_EPDCS, 0);
    __spi_write (&reg, 1);
    __gpio_set (DEV_EPDCS, 1);
}

static void DEV_epd2in9v2_send_byte (const uint8_t data) {

    __gpio_set (DEV_EPDDC, 1);
    __gpio_set (DEV_EPDCS, 0);
    __spi_write (&data, 1);
    __gpio_set (DEV_EPDCS, 1);
}

static void DEV_epd2in9v2_send_bytes (const uint8_t * const data, const int len) {

    __gpio_set (DEV_EPDDC, 1);
    __gpio_set (DEV_EPDCS, 0);
    __spi_write (data, len);
    __gpio_set (DEV_EPDCS, 1);
}

static void DEV_epd2in9v2_wait (void) {

    while (1) {     //=1 BUSY
        if (__gpio_get (DEV_EPDBSY)==0)
            break;
        __delay_us (10);
    }
}

// ------------------------------------------------------------------------------------------------

static void DEV_epd2in9v2_display_enable (void) {

    DEV_epd2in9v2_send_cmnd (0x22);
    DEV_epd2in9v2_send_byte (0xF7);
    DEV_epd2in9v2_send_cmnd (0x20);
    DEV_epd2in9v2_wait ();
}

static void DEV_epd2in9v2_display_enable_part (void) {

    DEV_epd2in9v2_send_cmnd (0x22);
    DEV_epd2in9v2_send_byte (0x0F);
    DEV_epd2in9v2_send_cmnd (0x20);
}

static void DEV_epd2in9v2_set_window (const int x, const int y, const int x_end, const int y_end) {

    DEV_epd2in9v2_send_cmnd (0x44);
    DEV_epd2in9v2_send_byte ((x>>3) & 0xFF);
    DEV_epd2in9v2_send_byte ((x_end>>3) & 0xFF);
    DEV_epd2in9v2_send_cmnd (0x45);
    DEV_epd2in9v2_send_byte (y & 0xFF);
    DEV_epd2in9v2_send_byte ((y >> 8) & 0xFF);
    DEV_epd2in9v2_send_byte (y_end & 0xFF);
    DEV_epd2in9v2_send_byte ((y_end >> 8) & 0xFF);
}

static void DEV_epd2in9v2_set_cursor (const int x, const int y) {

    DEV_epd2in9v2_send_cmnd (0x4E);
    DEV_epd2in9v2_send_byte (x & 0xFF);
    DEV_epd2in9v2_send_cmnd (0x4F);
    DEV_epd2in9v2_send_byte (y & 0xFF);
    DEV_epd2in9v2_send_byte ((y >> 8) & 0xFF);
}

// ------------------------------------------------------------------------------------------------

static void DEV_epd2in9v2_init (void) {

    DEV_epd2in9v2_black ();

    DEV_epd2in9v2_reset ();
    __delay (100);

    DEV_epd2in9v2_wait ();
    DEV_epd2in9v2_send_cmnd (0x12);
    DEV_epd2in9v2_wait ();

    DEV_epd2in9v2_send_cmnd (0x01);
    DEV_epd2in9v2_send_byte (0x27);
    DEV_epd2in9v2_send_byte (0x01);
    DEV_epd2in9v2_send_byte (0x00);

    DEV_epd2in9v2_send_cmnd (0x11);
    DEV_epd2in9v2_send_byte (0x03);

    DEV_epd2in9v2_set_window (0, 0, DEV_EPD2IN9V2_WIDTH - 1, DEV_EPD2IN9V2_HEIGHT - 1);
    DEV_epd2in9v2_set_cursor (0, 0);

    DEV_epd2in9v2_send_cmnd (0x21);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x80);

    DEV_epd2in9v2_wait ();
}

static void DEV_epd2in9v2_clear (void) {

    DEV_epd2in9v2_send_cmnd (0x24);
    DEV_epd2in9v2_send_bytes (__epd2in9v2_BLACK, sizeof (__epd2in9v2_BLACK));
    DEV_epd2in9v2_send_cmnd (0x26);
    DEV_epd2in9v2_send_bytes (__epd2in9v2_BLACK, sizeof (__epd2in9v2_BLACK));
    DEV_epd2in9v2_display_enable ();
}

// ------------------------------------------------------------------------------------------------

static void DEV_epd2in9v2_display_base (const uint8_t * const image, const int length) {

    DEV_epd2in9v2_send_cmnd (0x24);
    DEV_epd2in9v2_send_bytes (image, length);
    DEV_epd2in9v2_send_cmnd (0x26);
    DEV_epd2in9v2_send_bytes (image, length);
    DEV_epd2in9v2_display_enable ();
}

static void DEV_epd2in9v2_display_part (const uint8_t *const image, const int length) {

    DEV_epd2in9v2_send_cmnd (0x32);
    DEV_epd2in9v2_send_bytes (__epd2in9v2_WF_PARTIAL_2IN9, 159);
    DEV_epd2in9v2_wait ();

    DEV_epd2in9v2_send_cmnd (0x37);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x40);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x00);
    DEV_epd2in9v2_send_byte (0x00);

    DEV_epd2in9v2_send_cmnd (0x3C);
    DEV_epd2in9v2_send_byte (0x80);

    DEV_epd2in9v2_send_cmnd (0x22);
    DEV_epd2in9v2_send_byte (0xC0);
    DEV_epd2in9v2_send_cmnd (0x20);
    DEV_epd2in9v2_wait ();

    DEV_epd2in9v2_set_window (0, 0, DEV_EPD2IN9V2_WIDTH - 1, DEV_EPD2IN9V2_HEIGHT - 1);
    DEV_epd2in9v2_set_cursor (0, 0);

    DEV_epd2in9v2_send_cmnd (0x24);
    DEV_epd2in9v2_send_bytes (image, length);

    DEV_epd2in9v2_display_enable_part ();
}

// ------------------------------------------------------------------------------------------------

static void DEV_epd2in9v2_suspend (void) {

    DEV_epd2in9v2_send_cmnd (0x10);
    DEV_epd2in9v2_send_byte (0x01);
    __delay (100);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

void DEV_init (void) {

    __gpio_init ();
    __spi_init ();
    __i2c_init ();

    __icnt86x_init ();
    DEV_epd2in9v2_init ();
    DEV_epd2in9v2_clear ();
    __delay (100);
}

void DEV_exit (void) {

    DEV_epd2in9v2_init ();
    DEV_epd2in9v2_clear ();
    __delay (1000);
    DEV_epd2in9v2_suspend ();
    __gpio_set (DEV_EPDRST, 0);
    __gpio_set (DEV_GPIO_TP_RST, 0);
}

// ------------------------------------------------------------------------------------------------

bool DEV_key (int * const key_value) {

    bool result = false;

         if (__gpio_get (DEV_GPIO_KEY0) == 0) *key_value = 1, result = true;
    else if (__gpio_get (DEV_GPIO_KEY1) == 0) *key_value = 2, result = true;
    else if (__gpio_get (DEV_GPIO_KEY2) == 0) *key_value = 3, result = true;

    return result;
}

bool DEV_touch (int * const touch_x, int * const touch_y, int * const touch_p) {

    if (!__icnt86x_scan () || !__icnt86x_dev_now.count)
        return false;
    __icnt86x_dev_now.count = 0;

    if (__icnt86x_dev_now.X [0] == __icnt86x_dev_old.X [0] && __icnt86x_dev_now.Y [0] == __icnt86x_dev_old.Y [0])
        return false;

    *touch_x = (int) __icnt86x_dev_now.X [0];
    *touch_y = (int) __icnt86x_dev_now.Y [0];
    *touch_p = (int) __icnt86x_dev_now.P [0];

    return true;
}

void DEV_display (const uint8_t * const image, const int length, const bool full) {

    if (full) DEV_epd2in9v2_display_base (image, length);
    else DEV_epd2in9v2_display_part (image, length);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

