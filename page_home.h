
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

static __float_smooth_t cond_value__temp = { .sum = 0.0f, .idx = 0, .cnt = 0, .val = 0.0f };
static bool conditions_process__temp (float value) {
    value = __float_smooth (__float_clamp (value, 0.0f, 99.9f), &cond_value__temp);
    if (!__float_equals_one_place (value, cond_value__temp.val)) {
        bool r = display_content_temp (value, cond_value__temp.val);
        cond_value__temp.val = value;
        return r;
    }
    return false;
}
static float cond_value__setp = 0.0;
static bool conditions_process__setp (float value) {
    value = __float_clamp (value, 0.0f, 99.9f);
    if (!__float_equals_one_place (value, cond_value__setp)) {
        bool r = display_content_setp (value, cond_value__setp);
        cond_value__setp = value;
        return r;
    }
    return false;
}
static char cond_value__mode [128+1] = { '\0' };
static bool conditions_process__mode (const char * const value) {
    if (strcmp (value, cond_value__mode) != 0) {
        bool r = display_content_mode (value, cond_value__mode);
        cond_value__mode [sizeof (cond_value__mode) - 1] = '\0';
        strncpy (cond_value__mode, value, sizeof (cond_value__mode) - 1);
        return r;
    }
    return false;
}
static char cond_value__mesg [256] = { '\0' };
static bool conditions_process__mesg (const char * const value) {
    if (strcmp (value, cond_value__mesg) != 0) {
        cond_value__mesg [sizeof (cond_value__mesg) - 1] = '\0';
        strncpy (cond_value__mesg, value, sizeof (cond_value__mesg) - 1);
        return false;
    }
    return false;
}
static int cond_value__warn = 0;
static bool conditions_process__warn (const int value) {
    if (value != cond_value__warn) {
        if (cond_value__warn > 0 || value == 0)
            hardware_led_blink_stop ();
        if (value > 0)
            hardware_led_blink_start (value);
        cond_value__warn = value;
    }
    return false;
}
static bool conditions_process (const char * const name, const char * const value) {
         if (strcmp (name, "temp") == 0) return conditions_process__temp (strtof (value, NULL)); /*BLAH*/
    else if (strcmp (name, "setp") == 0) return conditions_process__setp (strtof (value, NULL));
    else if (strcmp (name, "mode") == 0) return conditions_process__mode (value);
    else if (strcmp (name, "mesg") == 0) return conditions_process__mesg (value);
    else if (strcmp (name, "warn") == 0) return conditions_process__warn (strtol (value, NULL, 10));
    return false;
}
static bool conditions_display (void) {
    bool updated = false;
    if (display_content_clear (corners_content)) updated = true;
    if (cond_value__temp.val != 0.0 && display_content_temp (cond_value__temp.val, 0.0)) updated = true;
    if (cond_value__setp != 0.0 && display_content_setp (cond_value__setp, 0.0)) updated = true;
    if (cond_value__mode [0] != '\0' && display_content_mode (cond_value__mode, "")) updated = true;
    return updated;
}

static bool home_page_command_handler__cond (const command_t command) {

    bool updated = false;

    const char * ptr = command.args;
    while (ptr != NULL && *ptr != '\0') {
        char name [16], value [128] = { '\0' };

        int len = 0; while (len < (int) (sizeof (name) - 1) && *ptr != '\0' && *ptr != '=')
            name [len ++] = *ptr ++;
        name [len] = '\0';
        while (*ptr != '=' && *ptr != '\0') ptr ++;
        if (*ptr == '\0') break;
        while (*ptr == '=') ptr ++;
        if (*ptr != '\0') {
            len = 0; while (len < (int) (sizeof (value) - 1) && *ptr != '\0' && *ptr != ';')
                value [len ++] = *ptr ++;
            value [len] = '\0';
            while (*ptr != ';' && *ptr != '\0') ptr ++;
            while (*ptr == ';') ptr ++;
        }
        if (name [0] != '\0')
            if (conditions_process (name, value)) updated = true;
    }

    return updated;
}

// ------------------------------------------------------------------------------------------------------------------------

static bool home_page_locked_setp (void) {
    return !(strcmp (cond_value__mode, "standby") == 0 || strcmp (cond_value__mode, "running") == 0);
}
static bool home_page_locked_mode (void) {
    return strcmp (cond_value__mode, "locked") == 0;
}
static bool home_page_update_setp (const float value) {
    char buf [24]; snprintf (buf, sizeof (buf) - 1, "%0.1f", value);
    command_t command = { command__set_temp.name, buf };
    console_send (command);
    return false;
}
static bool home_page_update_mode (const char * const mode) {
    command_t command = { command__set_mode.name, (char *) mode };
    console_send (command);
    return false;
}

// ------------------------------------------------------------------------------------------------------------------------

static bool home_page_action_handler__enter (void) {
    console_send (command__req_cond);
    return conditions_display ();
}
static bool home_page_action_handler__touch (void) {
    if (cond_value__warn)
        return page_change (&page_definition_mesg);
    return false;
}
static bool home_page_action_handler__timer (const uint32_t interval) {
    static uint32_t exceeded = 0;
    if (__exceeded (&exceeded, command_interval_req_cond, interval))
        console_send (command__req_cond);
    return false;
}
static bool home_page_button_handler__plus (void) {
    if (!home_page_locked_setp () && cond_value__setp < SETP_MAX)
        return home_page_update_setp (cond_value__setp + SETP_STEP);
    return false;
}
static bool home_page_button_handler__down (void) {
    if (!home_page_locked_setp () && cond_value__setp > SETP_MIN)
        return home_page_update_setp (cond_value__setp - SETP_STEP);
    return false;
}
static bool home_page_button_handler__mode (void) {
    if (!home_page_locked_mode ()) {
        if (strcmp (cond_value__mode, "protect") == 0 || strcmp (cond_value__mode, "running") == 0)
            return home_page_update_mode ("standby");
        else if (strcmp (cond_value__mode, "standby") == 0)
            return home_page_update_mode ("running");
    }
    return false;
}
static bool home_page_button_handler__ctrl (void) {
    return page_change (&page_definition_ctrl);
}
static bool ctrl_page_command_handler__ctrl (const command_t);
static bool home_page_command_handler (const command_t command) {
    if (strcmp (command.name, command__put_cond.name) == 0)
        return home_page_command_handler__cond (command);
    else if (strcmp (command.name, command__put_ctrl.name) == 0)
        return ctrl_page_command_handler__ctrl (command);
    return false;
}
static const page_definition_t page_definition_home = {
    "home", true, {
      { "setp plus", &font_icon_40x60, 'A', home_page_button_handler__plus },
      { "setp down", &font_icon_40x60, 'B', home_page_button_handler__down },
      { "page ctrl", NULL,             '\0', home_page_button_handler__ctrl },
      { "next mode", &font_icon_40x60, 'C', home_page_button_handler__mode }
    },
    home_page_action_handler__enter, NULL, home_page_action_handler__touch, home_page_action_handler__timer,
    home_page_command_handler
};

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

