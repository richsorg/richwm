#pragma once

#include "util.h"

extern xcb_connection_t *conn;
extern uint32_t values[3];
extern xcb_drawable_t win;

union {
    char **l;
    int d;
    char *s;
} typedef arg_t;

struct {
    unsigned int mod;
    xcb_keysym_t keysym;
    void (*func)();
    arg_t arg;

} typedef Key;

struct {
    uint32_t request;
    void (*func)(xcb_generic_event_t *ev);
} typedef handler_func_t;

xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t keysym);
xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode);

void eventHandler(void);
void spawn(arg_t arg);
void closewm(void);

void handle_key_press(xcb_generic_event_t *);
void handle_map_request(xcb_generic_event_t *);
void handle_motion_notify(xcb_generic_event_t *);
void handle_focus_in(xcb_generic_event_t *);
void handle_focus_out(xcb_generic_event_t *);

void handleEnterNotify(xcb_generic_event_t *);
void handleButtonRelease(xcb_generic_event_t *);
void handleButtonPress(xcb_generic_event_t *);
void handle_closewindow(xcb_generic_event_t *);

static handler_func_t handler_funcs[] = {
    { XCB_MOTION_NOTIFY,  handle_motion_notify },
    { XCB_ENTER_NOTIFY,   handleEnterNotify },
    { XCB_DESTROY_NOTIFY, NULL },
    { XCB_BUTTON_PRESS,   handleButtonPress },
    { XCB_BUTTON_RELEASE, handleButtonRelease },
    { XCB_KEY_PRESS,      handle_key_press},
    { XCB_MAP_REQUEST,    handle_map_request },
    { XCB_FOCUS_IN,       handle_focus_in },
    { XCB_FOCUS_OUT,      handle_focus_out },
    { XCB_NONE,           NULL }
};
