
// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

static const position_t corners_screen [2] = {
    { DISPLAY_X1, DISPLAY_Y1 }, { DISPLAY_X2, DISPLAY_Y2 }
};
static const position_t corners_content [2] = {
    { DISPLAY_CONTENT_X1, DISPLAY_CONTENT_Y1 }, { DISPLAY_CONTENT_X2, DISPLAY_CONTENT_Y2 },
};

static bool display_content_clear (const position_t corners [2]) {
    display_clear (corners);
    return true;
}

static const char * display_content_lines (const position_t corners [2], const char * const content, const int number) {

    debug ("display_content_lines: %d (%d)", (int) strlen (content), number);

    const int vspace = 0, hspace = 0, mono = false;
    const fontinfo_t * const font = &font_noto_mono6x10;

    int y = corners [0].y;

    const char * pointer = content;
    while (*pointer != '\0' && (y + font->height) < (corners [1].y - (vspace + font->height))) {

        int x = corners [0].x;

        while (*pointer != ';' && *pointer != '\0') {
            if ((x + font->width) < corners [1].x)
                x += display_char (font, *pointer, x, y, mono) + hspace;
            pointer ++;
        }
        while (*pointer == ';') pointer ++;
        y += (font->height + vspace);
    }

    char buffer [48]; /*XXX*/
    snprintf (buffer, sizeof (buffer) - 1, "[ page %d ... ]", number);
    display_string (&font_noto_mono6x10, buffer, corners [0].x, (corners [1].y - font->height), corners [1].x, mono, hspace, true);

    return *pointer == '\0' ? NULL : pointer;
}

// can be multi-line with ',', but note width issues ...
static void display_content_message (const position_t corners [2], const char * const content) {

    debug ("display_content_message: %d", (int) strlen (content));

    const int vspace = 0, hspace = 0, mono = false;
    const fontinfo_t * const font = &font_noto_vari20x28;

    int height = 1;
    const char * ptr = content;
    while (*ptr != '\0')
        if (*ptr ++ == ',')
            height ++;
    height *= (font->height + vspace);
    if (height > (corners [1].y - corners [0].y)) height = (corners [1].y - corners [0].y);

    int y = corners [0].y + (((corners [1].y - corners [0].y) - height) / 2);

    const char * pointer = content;
    while (*pointer != '\0') {

        int width = 0;
        const char * p2 = pointer;
        while (*p2 != ',' && *p2 != '\0') width += display_char_sizeof (font, *p2 ++, mono) + hspace;
        if (width > (corners [1].x - corners [0].x)) width = (corners [1].x - corners [0].x);
        int x = corners [0].x + (((corners [1].x - corners [0].x) - width) / 2);

        while (*pointer != ',' && *pointer != '\0') {
            if ((x + font->width) < corners [1].x)
                x += display_char (font, *pointer, x, y, mono) + hspace;
            pointer ++;
        }
        while (*pointer == ',') pointer ++;
        y += (font->height + vspace);
    }
}

static void __render_temp (const float value_curr, const float value_prev, const fontinfo_t * const font, int x, int y, const int off) { /*XXX*/

    char buf_curr [8]; snprintf (buf_curr, sizeof (buf_curr) - 1, "%04.1f", value_curr);
    char buf_prev [8]; snprintf (buf_prev, sizeof (buf_prev) - 1, "%04.1f", value_prev);

    if (value_prev == 0.0 || buf_prev [0] != buf_curr [0]) display_char (font, buf_curr [0] == '0' ? ':' : buf_curr [0], x, y, true);
    x += font->width + 1;
    if (value_prev == 0.0 || buf_prev [1] != buf_curr [1]) display_char (font, buf_curr [1], x, y, true);
    x += font->width + 1;
    if (value_prev == 0.0) x += display_char (font, '/', x, y, false); else x += display_char_sizeof (font, '/', false);
    x += 1;
    if (value_prev == 0.0 || buf_prev [3] != buf_curr [3]) display_char (font, buf_curr [3], x, y, true);
    x += font->width + 1;

    x += 2 - off;
    if (value_prev == 0.0) {
        x += display_char (font, '.', x, y + off, false);
        x += 1 + 2;
        display_char (font, '-', x, y + off, false);
    }
}
static bool display_content_temp (const float value, const float value_prev) {
    debug ("display_render: temp = %04.1f <-- %04.1f", value, value_prev);
    __render_temp (value, value_prev, &font_noto_mono40x56, DISPLAY_CONTENT_X1 + 12, DISPLAY_CONTENT_Y1 + 12, 0);
    return true;
}
static bool display_content_setp (const float value, const float value_prev) {
    debug ("display_render: setp = %04.1f <-- %04.1f", value, value_prev);
    __render_temp (value, value_prev, &font_noto_mono23x37, DISPLAY_CONTENT_X1 + 12, DISPLAY_CONTENT_Y1 + 12 + 70, 2);
    return true;
}
static bool display_content_mode (const char* const value, __attribute__ ((unused)) const char * const value_prev) {
    debug ("display_render: mode = %s <-- %s", value, value_prev);
    display_string (&font_noto_vari20x28, value, DISPLAY_CONTENT_X1 + 12 + (23*5) - 2, DISPLAY_CONTENT_Y1 + 12 + 61, corners_content [1].x, false, 0, true);
    return true;
}
static bool display_content_mesg (const char * const value, __attribute__ ((unused)) const char * const value_prev) {
    debug ("display_render: mesg = %s <-- %s", value, value_prev);
    display_content_message (corners_screen, value);
    return true;
}

static bool display_content_label (const int button, const char* line1, const char* line2) { // XXX only for lhs button 0 & 1 for now
    debug ("display_render: label (%d) = %s, %s\n", button, line1, (line2 ? line2 : "null"));
    const int y_offset = (line2 != NULL && *line2 != '\0') ? 4 : 14;
    if (line2 != NULL && *line2 != '\0')
        display_string (&font_noto_vari20x28, line2, DISPLAY_CONTENT_X1 + 12, DISPLAY_CONTENT_Y1 + (button % 2) * (DISPLAY_BUTTON_HEIGHT - DISPLAY_BUTTON_BORDER) + y_offset + 24, corners_content [1].x, false, 0, false);
    display_string (&font_noto_vari20x28, line1, DISPLAY_CONTENT_X1 + 12, DISPLAY_CONTENT_Y1 + (button % 2) * (DISPLAY_BUTTON_HEIGHT - DISPLAY_BUTTON_BORDER) + y_offset, corners_content [1].x, false, 0, false);
    return true;
}

static bool display_content_button (const position_t position, const fontinfo_t * const font, const char code) {
    debug ("display_content_button_data: (%d, %d) --> %d", position.x, position.y, code);
    display_char (font, code, position.x, position.y, true);
    return true;
}

static const position_t display_content_button_positions [PAGE_BUTTONS] = {
    { DISPLAY_BUTTON_X1, DISPLAY_BUTTON_Y1 }, { DISPLAY_BUTTON_X1, DISPLAY_BUTTON_Y2 },
    { DISPLAY_BUTTON_X2, DISPLAY_BUTTON_Y1 }, { DISPLAY_BUTTON_X2, DISPLAY_BUTTON_Y2 - 1 } };

static bool display_content_touched (const position_t corners [2], const position_t touched) {
    if (touched.x >= corners [0].x && touched.x <= corners [1].x && touched.y >= corners [0].y && touched.y <= corners [1].y) {
        debug ("display_content_touched: [%d ... %d, %d ... %d] --> (%d, %d)", corners [0].x, corners [1].x, corners [0].y, corners [1].y, touched.x, touched.y);
        return true;
    }
    return false;
}

// ------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------

