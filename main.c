
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#define DEBUG

// ------------------------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "utilities.h"
#include "iodevice.h"
#include "hardware.h"
#include "display.h"

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#define CONSOLE_TIMEOUT (30*1000)
#define CONSOLE_BUFFER 8192
#define PROCESSING_PERIOD_MS 200
#define DISPLAY_BUTTON_MINIMUM_MSECS 1000
#define DIAG_PAGE_TIMEOUT (30*1000)
#define CTRL_PAGE_TIMEOUT (30*1000)
#define MESG_PAGE_TIMEOUT (30*1000)
#define SETP_STEP 0.5
#define SETP_MAX 40.0
#define SETP_MIN 26.0

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#define PAGE_BUTTONS 4

#define DISPLAY_BUTTON_BORDER         4
#define DISPLAY_BUTTON_WIDTH          48
#define DISPLAY_BUTTON_HEIGHT         64

#define DISPLAY_BORDER_SZ             2
#define DISPLAY_X1                    (0 + DISPLAY_BORDER_SZ)
#define DISPLAY_Y1                    (0 + DISPLAY_BORDER_SZ)
#define DISPLAY_X2                    (DISPLAY_SCREEN_WIDTH - DISPLAY_BORDER_SZ)
#define DISPLAY_Y2                    (DISPLAY_SCREEN_HEIGHT - DISPLAY_BORDER_SZ)

#define DISPLAY_BUTTON_W              40
#define DISPLAY_BUTTON_H              60
#define DISPLAY_BUTTON_BORDER_SZ      2

#define DISPLAY_BUTTON_X1             (DISPLAY_X1 + DISPLAY_BUTTON_BORDER_SZ)
#define DISPLAY_BUTTON_Y1             (DISPLAY_Y1 + DISPLAY_BUTTON_BORDER_SZ)
#define DISPLAY_BUTTON_X2             (DISPLAY_X2 - DISPLAY_BUTTON_BORDER_SZ - DISPLAY_BUTTON_W)
#define DISPLAY_BUTTON_Y2             (DISPLAY_Y2 - DISPLAY_BUTTON_BORDER_SZ - DISPLAY_BUTTON_H)

#define DISPLAY_CONTENT_X1            (DISPLAY_BUTTON_X1 + DISPLAY_BUTTON_W + DISPLAY_BUTTON_BORDER_SZ)
#define DISPLAY_CONTENT_Y1            (DISPLAY_BUTTON_Y1 - DISPLAY_BUTTON_BORDER_SZ)
#define DISPLAY_CONTENT_X2            (DISPLAY_BUTTON_X2 - DISPLAY_BUTTON_BORDER_SZ)
#define DISPLAY_CONTENT_Y2            (DISPLAY_BUTTON_Y2 + DISPLAY_BUTTON_H + DISPLAY_BUTTON_BORDER_SZ)

#include "main_display.h"

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

typedef const char * name_t;

typedef const char * command_arguments_t;
typedef struct {
    name_t name;
    command_arguments_t args;
} command_t;

typedef bool (* const command_handler_t) (const command_t);
typedef struct {
    name_t name;
    command_handler_t handler;
} command_definition_t;

static void console_send (const command_t);

// ------------------------------------------------------------------------------------------------------------------------

typedef bool (* const button_handler_t) (void);
typedef struct {
    name_t name;
    const fontinfo_t * font;
    char code;
    button_handler_t handler;
} button_definition_t;

typedef bool (* const page_event_handler_t) (void);
typedef bool (* const page_timer_handler_t) (const uint32_t);
typedef struct {
    name_t name;
    bool buttons_enabled;
    button_definition_t buttons [PAGE_BUTTONS];
    page_event_handler_t action_enter, action_leave, action_touch;
    page_timer_handler_t action_timer;
    command_handler_t action_command;
} page_definition_t;

static bool page_change (const page_definition_t * const page);

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

static const page_definition_t page_definition_home;
static const page_definition_t page_definition_mesg;
static const page_definition_t page_definition_diag;
static const page_definition_t page_definition_ctrl;

static const command_t command__pat      = { "qqq",      NULL };
static const command_t command__req_diag = { "get-diag", NULL };
static const command_t command__req_cond = { "get-cond", NULL };
static const command_t command__put_ctrl = { "put-ctrl", NULL };
static const command_t command__put_cond = { "put-cond", NULL };
static const command_t command__put_diag = { "put-diag", NULL };
static const command_t command__ack_warn = { "ack-warn", NULL };
static const command_t command__set_mode = { "set-mode", NULL };
static const command_t command__set_temp = { "set-temp", NULL };
static const command_t command__put_logs = { "put-logs", NULL };

static const uint32_t command_interval_req_cond = 15 * 1000;
static const uint32_t command_interval_pat      = 15 * 1000;
static const uint32_t command_interval_put_cond = 15 * 1000;

#include "page_home.h"
#include "page_ctrl.h"
#include "page_diag.h"
#include "page_mesg.h"

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

#include "main_page.h"
#include "main_console.h"

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

int main (__attribute__ ((unused)) int argc, __attribute__ ((unused)) const char *argv []) {

    hardware_open ();
    display_open ();
    console_open ();
    page_open ();

    while (1) {

        hardware_update ();

        position_t touched;
        if (display_obtain__touched (&touched))
            page_handler__touched (touched);

        sidekey_t sidekey;
        if (display_obtain__sidekey (&sidekey))
            page_handler__sidekey (sidekey);

        command_t command;
        if (console_obtain__command (&command))
            page_handler__command (command);
        if (console_timeout (true))
            hardware_reboot ();

        page_update ();

        sleep_ms (PROCESSING_PERIOD_MS);
    }

    page_close ();
    console_close ();
    display_close ();
    hardware_close ();

    return (EXIT_SUCCESS);
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

