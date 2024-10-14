
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

static bool serial_getline (char * const serial_buffer_data, const int serial_buffer_max, int * const serial_buffer_size) {

    int c;

    while ((c = getchar_timeout_us (0)) != PICO_ERROR_TIMEOUT) {

        if (c > 0 && c < 255) {

            if ((c == '\n' || c == '\r') || (*serial_buffer_size) == (serial_buffer_max - 1)) {
                if ((*serial_buffer_size) > 0) {
                    serial_buffer_data [(*serial_buffer_size)] = '\0';
                    (*serial_buffer_size) = 0;
                    return true;
                }
            } else
                serial_buffer_data [(*serial_buffer_size) ++] = (char) c;
        }
    }

    return false;
}

// ------------------------------------------------------------------------------------------------------------------------

static void console_send (const command_t command) {

    fprintf (stdout, command.args != NULL ? "%s %s\n" : "%s\n", command.name, command.args);
}

static bool console_recv (command_t * const command, char * const string) {

    char * pointer = string;
    command->name = pointer;
    while (*pointer != '\0' && *pointer != ' ')
        pointer ++;
    if (*pointer == ' ') {
        *pointer ++ = '\0';
        while (*pointer == ' ')
            pointer ++;
    }

    if (command->name [0] == '\0' || command->name [0] == ' ')
        return false;

    command->args = pointer;

    return true;
}

static void console_open (void) {

    stdio_init_all ();
}

static void console_close (void) {
}

static bool console_timeout (const bool hastimedout) {

    static uint32_t prev = 0;
    uint32_t curr = (uint32_t) (time_us_64 () / 1000);

    if (!hastimedout)
        prev = curr;
    else if ((curr - prev) > CONSOLE_TIMEOUT)
        return true;

    return false;
}

static bool console_obtain__command (command_t * const command) {

    static char data [CONSOLE_BUFFER];
    static int offs = 0;

    if (serial_getline (data, CONSOLE_BUFFER, &offs) && console_recv (command, data)) {
        console_timeout (false);
        return true;
    }

    return false;
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

