
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"

#include "hardware.h"

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#define HARDWARE_ADC_INPUT_TEMP     4
#define WATCHDOG_PERIOD_MS          5000
#define HARDWARE_GPIO_LED_PIN       14

// ------------------------------------------------------------------------------------------------------------------------

#define v_ref    3.3f
#define v_res    (1 << 12)

float hardware_temp_get (void) {
    return (float) (27.0f - (((float) adc_read () * (v_ref / (float) v_res)) - 0.706f) / 0.001721f);
}

static void hardware_temp_init (void) {
    adc_init ();
    adc_set_temp_sensor_enabled (true);
    adc_select_input (HARDWARE_ADC_INPUT_TEMP);
}

static void hardware_temp_term (void) {
    adc_set_temp_sensor_enabled (false);
}

// ------------------------------------------------------------------------------------------------------------------------

static volatile bool hardware_led_state = false;
static volatile bool hardware_led_blink = false;
static struct repeating_timer hardware_led_blink_timer;

void hardware_led_toggle (void) {
    hardware_led_state = !hardware_led_state;
    gpio_put (HARDWARE_GPIO_LED_PIN, hardware_led_state);
}

void hardware_led_enable (const bool state) {
    hardware_led_state = state;
    gpio_put (HARDWARE_GPIO_LED_PIN, hardware_led_state);
}

static bool hardware_led_blink_timer_handler (__attribute__ ((unused)) struct repeating_timer * timer) {
    if (hardware_led_blink) hardware_led_toggle ();
    else hardware_led_enable (false);
    return hardware_led_blink;
}

void hardware_led_blink_start (const int millihertz) {
    if (!hardware_led_blink) {
        hardware_led_blink = true;
        add_repeating_timer_ms (millihertz, hardware_led_blink_timer_handler, NULL, &hardware_led_blink_timer);
    }
}

void hardware_led_blink_stop (void) {
    hardware_led_blink = false;
}

static void hardware_led_init (void) {
    gpio_init (HARDWARE_GPIO_LED_PIN);
    gpio_set_dir (HARDWARE_GPIO_LED_PIN, GPIO_OUT);
    gpio_put (HARDWARE_GPIO_LED_PIN, 0);
}

static void hardware_led_term (void) {
    hardware_led_blink_stop ();
}

// ------------------------------------------------------------------------------------------------------------------------

static const char * __watchdog_reboot = NULL;
static bool __watchdog_enabled = false;

const char * hardware_watchdog_reboot (void) {
    if (__watchdog_reboot == NULL)
        return NULL;
    const char * const x = __watchdog_reboot;
    __watchdog_reboot = NULL;
    return x;
}

static void hardware_watchdog_update (void) {
    if (!__watchdog_enabled)
        watchdog_enable (WATCHDOG_PERIOD_MS * 1000, false), __watchdog_enabled = true;
    watchdog_update ();
}

static void hardware_watchdog_init (void) {
    if (watchdog_caused_reboot ()) __watchdog_reboot = "system";
    else if (watchdog_enable_caused_reboot ()) __watchdog_reboot = "program";
}

// ------------------------------------------------------------------------------------------------------------------------

void hardware_open (void) {
    hardware_temp_init ();
    hardware_led_init ();
    hardware_watchdog_init ();
}

void hardware_close (void) {
    hardware_temp_term ();
    hardware_led_term ();
}

void hardware_update (void) {
    hardware_watchdog_update ();
}

void hardware_reboot (void) { /* watchdog */
    while (1) sleep_ms (10000);
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

