
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

static const page_definition_t * page_current;
static const page_definition_t * page_pending;

static void page_handler__command (const command_t command) {

    if (strcmp (command.name, command__pat.name) == 0)
        return;

    if (page_current->action_command (command))
        display_update ();
}

static void page_handler__touched (const position_t position) {

    static uint32_t spaced = 0;
    if (!__isspaced (&spaced, DISPLAY_BUTTON_MINIMUM_MSECS))
        return;

    if (display_content_touched (page_current->buttons_enabled ? corners_content : corners_screen, position)) {

        debug ("page_content: touched");
        if (page_current->action_touch != NULL)
            if (page_current->action_touch ())
                display_update ();

    } else if (page_current->buttons_enabled) {

        for (int i = 0; i < PAGE_BUTTONS; i ++) {
            const position_t position_button [2] = {
                display_content_button_positions [i],
                { display_content_button_positions [i].x + DISPLAY_BUTTON_W, display_content_button_positions [i].y + DISPLAY_BUTTON_H }
            };
            if (display_content_touched (position_button, position)) {

                debug ("page_button: touched (%s)", page_current->buttons [i].name);
                if (page_current->buttons [i].handler != NULL)
                    if (page_current->buttons [i].handler ())
                        display_update ();

                return;
            }
        }
    }
}

static void page_handler__sidekey (__attribute__ ((unused)) sidekey_t sidekey) {

    static uint32_t spaced = 0;
    if (!__isspaced (&spaced, DISPLAY_BUTTON_MINIMUM_MSECS))
        return;

    /* not used */
}

static bool page_handler__timer (struct repeating_timer * const t) {

    if (page_current->action_timer != NULL)
        if (page_current->action_timer ((uint32_t) (t->delay_us / 1000)))
            display_update ();

    return true;
}

static bool page_handler__timer_pat (__attribute__ ((unused)) struct repeating_timer * const t) {

    console_send (command__pat);

    const char * const watchdog_rebooted = hardware_watchdog_reboot ();
    if (watchdog_rebooted != NULL) {
        char buf [64];
        snprintf (buf, sizeof (buf) - 1, "note: watchdog_reboot (%s)", watchdog_rebooted);
        command_t command = { command__put_logs.name, buf };
        console_send (command);
    }

    return true;
}

static bool page_handler__timer_cond (__attribute__ ((unused)) struct repeating_timer * const t) {
    char buf [20];
    snprintf (buf, sizeof (buf) - 1, "temp=%0.2f", hardware_temp_get ());
    command_t command = { command__put_cond.name, buf };
    console_send (command);
    return true;
}

static const uint32_t page_interval = 1000;
static struct repeating_timer page_content__timer;
static struct repeating_timer page_content__timer_pat;
static struct repeating_timer page_content__timer_cond;
static void page_timers_start (void) {
    add_repeating_timer_ms (page_interval, page_handler__timer, NULL, &page_content__timer);
    add_repeating_timer_ms (command_interval_pat, page_handler__timer_pat, NULL, &page_content__timer_pat);
    add_repeating_timer_ms (command_interval_put_cond, page_handler__timer_cond, NULL, &page_content__timer_cond);
}
static void page_timers_stop (void) {
    cancel_repeating_timer (&page_content__timer);
    cancel_repeating_timer (&page_content__timer_pat);
    cancel_repeating_timer (&page_content__timer_cond);
}

static void page_init (void) {
}

static bool page_enter (const page_definition_t * const page) {
    bool updated = false;
    if (page->buttons_enabled) {
        for (int i = 0; i < PAGE_BUTTONS; i ++)
            if (page->buttons [i].font != NULL)
                if (display_content_button (display_content_button_positions [i], page->buttons [i].font, page->buttons [i].code)) updated = true;
    }
    if (page->action_enter != NULL)
        if (page->action_enter ()) updated = true;
    page_current = page;
    return updated;
}

static bool page_leave (void) {
    if (page_current->action_leave != NULL)
        return page_current->action_leave ();
    return false;
}

static bool page_change (const page_definition_t * const page) {
    page_pending = page;
    return false;
}
static bool __page_change (const page_definition_t * const page) {
    bool updated = false;
    if (page != page_current) {
        debug ("page_change: %s --> %s", page_current->name, page->name);
        if (page_leave ()) updated = true;
        if (page_enter (page)) updated = true;
    }
    return updated;
}

static void page_open (void) {
    page_init ();
    page_current = page_pending = NULL;
    page_enter (&page_definition_home);
    display_update (); /* always */
    page_timers_start ();
}

static void page_close (void) {
    page_timers_stop ();
    page_leave ();
    display_update (); /* always */
}

static void page_update (void) {
    bool updated = false;
    if (page_pending != NULL) {
        const page_definition_t * page = page_pending;
        page_pending = NULL;
        if (__page_change (page)) updated = true;
    }
    if (updated)
        display_update ();
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

