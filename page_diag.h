
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

static char diag_page_display__buffer [CONSOLE_BUFFER];
static const char * diag_page_display__offset = NULL;
static int diag_page_display__number = 0;

static bool diag_page_command_handler__diag (const command_t command) {
    diag_page_display__buffer [sizeof (diag_page_display__buffer) - 1] = '\0';
    strncpy (diag_page_display__buffer, command.args, sizeof (diag_page_display__buffer) - 1);
    display_content_clear (corners_screen);
    diag_page_display__offset = display_content_lines (corners_screen, diag_page_display__buffer, diag_page_display__number = 1);
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------

static bool diag_page_action_handler__enter (void) {
    console_send (command__req_diag);
    if (cond_value__warn)
        console_send (command__ack_warn);
    diag_page_display__number = 0;
    return display_content_clear (corners_screen);
}
static bool diag_page_action_handler__leave (void) {
    return display_content_clear (corners_screen);
}
static uint32_t __diag_exceeded = 0;
static bool diag_page_action_handler__timer (const uint32_t interval) {
    if (__exceeded (&__diag_exceeded, DIAG_PAGE_TIMEOUT, interval))
        return page_change (&page_definition_home);
    return false;
}
static bool diag_page_button_handler__next (void) {
    __diag_exceeded = 0;
    if (diag_page_display__number > 0) {
        if (diag_page_display__offset == NULL || *diag_page_display__offset == '\0') {
            diag_page_display__offset = diag_page_display__buffer;
            diag_page_display__number = 0;
        }
        display_content_clear (corners_screen);
        diag_page_display__offset = display_content_lines (corners_screen, diag_page_display__offset, ++ diag_page_display__number);
        return true;
    }
    return false;
}
static bool diag_page_command_handler (const command_t command) {
    if (strcmp (command.name, command__put_diag.name) == 0)
        return diag_page_command_handler__diag (command);
    return false;
}
static const page_definition_t page_definition_diag = {
    "diag", false, {
      { NULL, NULL, 0, NULL },
      { NULL, NULL, 0, NULL },
      { NULL, NULL, 0, NULL },
      { NULL, NULL, 0, NULL }
    },
    diag_page_action_handler__enter, diag_page_action_handler__leave, diag_page_button_handler__next, diag_page_action_handler__timer,
    diag_page_command_handler
};


// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

