
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

static bool mesg_page_action_handler__enter (void) {
    display_content_clear (corners_screen);
    display_content_mesg (cond_value__mesg [0] != '\0' ? cond_value__mesg : "no message", "");
    return true;
}
static bool mesg_page_action_handler__leave (void) {
    if (cond_value__warn)
        console_send (command__ack_warn);
    return display_content_clear (corners_screen);
}
static uint32_t __mesg_exceeded = 0;
static bool mesg_page_action_handler__timer (uint32_t interval) {
    if (__exceeded (&__mesg_exceeded, MESG_PAGE_TIMEOUT, interval))
        return page_change (&page_definition_home);
    return false;
}
static bool mesg_page_button_handler__close (void) {
    return page_change (&page_definition_home);
}
static const page_definition_t page_definition_mesg = {
    "mesg", false,
    { { NULL, NULL, 0, NULL }, { NULL, NULL, 0, NULL }, { NULL, NULL, 0, NULL }, { NULL, NULL, 0, NULL } },
    mesg_page_action_handler__enter, mesg_page_action_handler__leave, mesg_page_button_handler__close, mesg_page_action_handler__timer,
    NULL
};

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

