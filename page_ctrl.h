
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

typedef struct ctrl_page_action {
    const char *line1, *line2;
    void (*update) (struct ctrl_page_action*, const char *);
    void (*action) (struct ctrl_page_action*);
    const void *stuff;
} ctrl_page_action_t;
typedef void (* ctrl_handler_update_t) (ctrl_page_action_t*, const char *);
typedef void (* ctrl_handler_action_t) (ctrl_page_action_t*);

// ------------------------------------------------------------------------------------------------------------------------

static bool diag_value_transmit (const char *ctrl) {
    command_t command = { command__put_ctrl.name, ctrl };
    console_send (command);
    return false;
}
static bool diag_value_extract (const char *args, const char *variable, char *result, size_t result_size) {
    const char *start = args;
    size_t var_len = strlen (variable);
    while ((start = strstr (start, variable)) != NULL) {
        if ((start == args || *(start - 1) == ';') && start [var_len] == '=') {
            start += var_len + 1;
            const char *end = strchr (start, ';');
            if (end == NULL)
                end = start + strlen (start);
            size_t value_length = end - start;
            if (value_length >= result_size)
                value_length = result_size - 1;
            strncpy (result, start, value_length);
            result [value_length] = '\0';
            return true;
        }
        start ++;
    }
    return false;
}
static bool diag_value_insert (const char *variable, const char *result, char* args, size_t args_size) {
    snprintf (args, args_size, "%s=%s", variable, result);
    args [args_size - 1] = '\0';
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------

typedef struct {
    const char *init1, *init2, *name, **values, **line1s, **line2s;
    int size;
} ctrl_page_variables_t;

static void ctrl_page_variables_debug (const ctrl_page_variables_t *ctrl) {
    debug ("[variables] ('%s', '%s')", ctrl->init1, ctrl->init2);
    for (int i = 0; i < ctrl->size; i ++)
        debug ("  %d: ('%s', '%s') --> %s=%s", i, ctrl->line1s [i], ctrl->line2s [i], ctrl->name, ctrl->values [i]);
}
static void ctrl_page_variables_update (ctrl_page_action_t *ctrl, const char *args) {
    const ctrl_page_variables_t *var = (const ctrl_page_variables_t *) ctrl->stuff;
    char value [64];
    if (diag_value_extract (args, var->name, value, sizeof (value)))
    for (int i = 0; i < var->size; i ++) {
        if (strcmp (value, var->values [i]) == 0) {
            ctrl->line1 = var->line1s [i];
            ctrl->line2 = var->line2s [i];
            return;
        }
    }
}
static void ctrl_page_variables_action (ctrl_page_action_t *ctrl) {
    const ctrl_page_variables_t *var = (const ctrl_page_variables_t *) ctrl->stuff;
    char args [64];
    for (int i = 0; i < var->size; i ++) {
        if (strcmp (ctrl->line2, var->line2s [i]) == 0) {
            diag_value_insert (var->name, var->values [(i + 1) % var->size], args, sizeof (args));
            diag_value_transmit (args);
            return;
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------

typedef struct {
    const char *init1, *init2, *name, *value, *line1, *line2;
} ctrl_page_transmits_t;

static void ctrl_page_transmits_debug (const ctrl_page_transmits_t *ctrl) {
    debug ("[transmits] ('%s', '%s'} --> %s=%s --> ('%s', '%s')", ctrl->init1, ctrl->init2, ctrl->name, ctrl->value, ctrl->line1, ctrl->line2);
}
static void ctrl_page_transmits_update (ctrl_page_action_t *, const char*) {
}
static void ctrl_page_transmits_action (ctrl_page_action_t *ctrl) {
    const ctrl_page_transmits_t *var = (const ctrl_page_transmits_t *) ctrl->stuff;
    char args [64];
    ctrl->line1 = var->line1;
    ctrl->line2 = var->line2;
    diag_value_insert (var->name, var->value, args, sizeof (args));
    diag_value_transmit (args);
}

// ------------------------------------------------------------------------------------------------------------------------

#define CTRL_PAGE_ACTION_SIZE 32
static int ctrl_page_action_size = 0;
static int ctrl_page_action_max = CTRL_PAGE_ACTION_SIZE;
static ctrl_page_action_t ctrl_page_action_list [CTRL_PAGE_ACTION_SIZE];

// ------------------------------------------------------------------------------------------------------------------------

bool ctrl_page_install_ctrl_page_variables (void *stuff, const int index, const char* part) {
    ctrl_page_variables_t* ctrl = (ctrl_page_variables_t *) stuff;
    if (index < 4)
        switch (index) {
            case -1:
                    ctrl_page_variables_debug (ctrl); break;
            case 0: ctrl->init1 = strdup (part); break;
            case 1: ctrl->init2 = strdup (part); break;
            case 2: ctrl->name = strdup (part); break;
            case 3: ctrl->size = atoi (part);
                    ctrl->values = (const char **) malloc (sizeof (const char **) * ctrl->size);
                    ctrl->line1s = (const char **) malloc (sizeof (const char **) * ctrl->size);
                    ctrl->line2s = (const char **) malloc (sizeof (const char **) * ctrl->size);
                    break;
        }
    else if (((index - 4) - (0 * ctrl->size)) < ctrl->size)
        ctrl->values [(index - 4) - (0 * ctrl->size)] = strdup (part);
    else if (((index - 4) - (1 * ctrl->size)) < ctrl->size)
        ctrl->line1s [(index - 4) - (1 * ctrl->size)] = strdup (part);
    else if (((index - 4) - (2 * ctrl->size)) < ctrl->size)
        ctrl->line2s [(index - 4) - (2 * ctrl->size)] = strdup (part);
    return true;
}
bool ctrl_page_install_ctrl_page_transmits (void *stuff, const int index, const char* part) {
    ctrl_page_transmits_t* ctrl = (ctrl_page_transmits_t *) stuff;
    switch (index) {
        case -1:
                ctrl_page_transmits_debug (ctrl); break;
        case 0: ctrl->init1 = strdup (part); break;
        case 1: ctrl->init2 = strdup (part); break;
        case 2: ctrl->name = strdup (part); break;
        case 3: ctrl->value = strdup (part); break;
        case 4: ctrl->line1 = strdup (part); break;
        case 5: ctrl->line2 = strdup (part); break;
    }
    return true;
}

typedef bool (* ctrl_page_install_handler_t) (void *, const int, const char *);
bool ctrl_page_install_ctrl__populate (const ctrl_page_install_handler_t handler, void *ctrl, const char* command) {
    debug ("ctrl_page_install_ctrl -> %s", command);
    char buffer [512];
    strncpy (buffer, command, sizeof (buffer) - 1);
    int index = 0;
    char* part = strtok (buffer, ",");
    while (part != NULL) {
        if (!handler (ctrl, index, strcmp (part, "-") == 0 ? "" : part))
            return false;
        part = strtok (NULL, ",");
        index ++;
    }
    if (!handler (ctrl, -1, NULL))
        return false;
    return true;
}
void ctrl_page_install_ctrl__install (const ctrl_handler_update_t update, const ctrl_handler_action_t action, const void *stuff, const size_t stuff_size, const char *init1, const char* init2) {
    ctrl_page_action_t* entry = &ctrl_page_action_list [ctrl_page_action_size ++];
    entry->line1 = init1;
    entry->line2 = init2;
    entry->update = update;
    entry->action = action;
    void *stuff2 = (void *) malloc (stuff_size);
    memcpy (stuff2, stuff, stuff_size);
    entry->stuff = stuff2;
}

static bool ctrl_page_command_handler__ctrl (const command_t command) {
    if (ctrl_page_action_size >= ctrl_page_action_max) {
        ;
    } else if (strncmp (command.args, "variables=", sizeof ("variables=") - 1) == 0) {
        ctrl_page_variables_t ctrl;
        if (ctrl_page_install_ctrl__populate (ctrl_page_install_ctrl_page_variables, (void *)&ctrl, &command.args [sizeof ("variables=") - 1]))
            ctrl_page_install_ctrl__install (ctrl_page_variables_update, ctrl_page_variables_action, (void *)&ctrl, sizeof (ctrl), ctrl.init1, ctrl.init2);
    } else if (strncmp (command.args, "transmits=", sizeof ("transmits=") - 1) == 0) {
        ctrl_page_transmits_t ctrl;
        if (ctrl_page_install_ctrl__populate (ctrl_page_install_ctrl_page_transmits, (void *)&ctrl, &command.args [sizeof ("transmits=") - 1]))
            ctrl_page_install_ctrl__install (ctrl_page_transmits_update, ctrl_page_transmits_action, (void *)&ctrl, sizeof (ctrl), ctrl.init1, ctrl.init2);
    }
    return false;
}

// ------------------------------------------------------------------------------------------------------------------------

static void ctrl_page_display__cmdx (const int index, const int offset) {
    debug ("ctrl_page_display: %d", index);
    if (index < ctrl_page_action_size)
        display_content_label (offset, ctrl_page_action_list [index].line1, ctrl_page_action_list [index].line2);
}
static bool ctrl_page_button_handler__cmdx (const int index) {
    debug ("ctrl_page_button: %d", index);
    if (index < ctrl_page_action_size)
        if (ctrl_page_action_list [index].action != NULL)
            ctrl_page_action_list [index].action (&ctrl_page_action_list [index]);
    return false;
}

static void ctrl_page_update (const char *args) {
    for (int index = 0; index < ctrl_page_action_size; index ++)
        if (ctrl_page_action_list [index].update != NULL)
            ctrl_page_action_list [index].update (&ctrl_page_action_list [index], args);
}

static int ctrl_page_display__index = 0;
static void ctrl_page_display__start () { ctrl_page_display__index = 0; }
static void ctrl_page_display__next () { ctrl_page_display__index += 2; if (ctrl_page_display__index > ctrl_page_action_size) ctrl_page_display__index = 0; }

static void ctrl_page_display (void) { ctrl_page_display__cmdx (ctrl_page_display__index + 0, 0); ctrl_page_display__cmdx (ctrl_page_display__index + 1, 1); }
static bool ctrl_page_button_handler__cmd1 (void) { return ctrl_page_button_handler__cmdx (ctrl_page_display__index + 0); }
static bool ctrl_page_button_handler__cmd2 (void) { return ctrl_page_button_handler__cmdx (ctrl_page_display__index + 1); }

// ------------------------------------------------------------------------------------------------------------------------

static bool ctrl_page_action_handler__enter (void) {
    console_send (command__req_diag);
    ctrl_page_display__start ();
    return display_content_clear (corners_content);
}
static bool ctrl_page_action_handler__leave (void) {
    return display_content_clear (corners_screen);
}
static uint32_t __ctrl_exceeded = 0;
static bool ctrl_page_action_handler__timer (const uint32_t interval) {
    if (__exceeded (&__ctrl_exceeded, CTRL_PAGE_TIMEOUT, interval))
        return page_change (&page_definition_home);
    return false;
}
static bool ctrl_page_button_handler__diag (void) {
    return page_change (&page_definition_diag);
}
static bool ctrl_page_button_handler__next (void) {
    __ctrl_exceeded = 0;
    ctrl_page_display__next ();
    display_content_clear (corners_content);
    ctrl_page_display ();
    return true;
}

static bool ctrl_page_command_handler (const command_t command) {
    if (strcmp (command.name, command__put_diag.name) == 0) {
        ctrl_page_update (command.args);
        display_content_clear (corners_content);
        ctrl_page_display ();
        return true;
    } else if (strcmp (command.name, command__put_ctrl.name) == 0)
        return ctrl_page_command_handler__ctrl (command);
    return false;
}
static const page_definition_t page_definition_ctrl = {
    "ctrl", true, {
      { "ctrl cmd1", &font_icon_40x60, 'E', ctrl_page_button_handler__cmd1 },
      { "ctrl cmd2", &font_icon_40x60, 'E', ctrl_page_button_handler__cmd2 },
      { "page diag", &font_icon_40x60, 'D', ctrl_page_button_handler__diag },
      { "page next", &font_icon_40x60, 'F', ctrl_page_button_handler__next },
    },
    ctrl_page_action_handler__enter, ctrl_page_action_handler__leave, NULL, ctrl_page_action_handler__timer,
    ctrl_page_command_handler
};

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

