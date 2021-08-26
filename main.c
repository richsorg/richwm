#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xinerama.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>

#include "monitor.h"
#include "util.h"
#include "bool.h"
#include "tiling.h"
#include "keys.h"
#include "wins.h"
#include "debug.h"
#include "config.h"
#include "main.h"

monitor_t mons[MONITORS];
monitor_t *mon = &mons[0];

xcb_connection_t *conn;
xcb_screen_t *screen;
xcb_drawable_t win;
uint32_t values[3];
uint8_t button;

int old_root_x = 0, old_root_y, screen_count;

void spawn(arg_t arg) {
	pid_t pid = fork();

	if (pid != 0)
		return;

	setsid();
	execvp(arg.l[0], arg.l);
	_exit(1);
}

void closewm(void) {
	if (!conn)
		xcb_disconnect(conn);
}

void handle_key_press(xcb_generic_event_t *ev) {
	xcb_key_press_event_t *e = (xcb_key_press_event_t *)ev;
	xcb_keysym_t keysym = xcb_get_keysym(e->detail);
	win = e->child;

	for (int i = 0; i < LEN(keys); ++i) {
		Key *key = &keys[i];

		if (key->keysym == keysym && key->mod == e->state) {
			key->func(key->arg);
		}
	}	
}

void set_focus(xcb_drawable_t window) {
	if (window) {
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window, 
			XCB_CURRENT_TIME);
	}
}

void setFocusColor(xcb_window_t window, int focus) {
	if ((BORDER_WIDTH > 0) && (screen->root != window) && (0 != window)) {
		uint32_t vals[1];
		vals[0] = focus ? BORDER_COLOR_FOCUSED : BORDER_COLOR_UNFOCUSED;
		xcb_change_window_attributes(conn, window, XCB_CW_BORDER_PIXEL, vals);
		xcb_flush(conn);
	}
}

void handle_motion_tile(window_t *window) {
	if (!window->tiling) {
		xcb_map_request_event_t e = {.window = window->win};
		handle_map_request((void *)&e);
		del_window(window->win);
	}
}

void handle_motion_size(window_t *window, xcb_query_pointer_reply_t *poin) {
	uint32_t skid[5] = {0};

	xcb_get_geometry_cookie_t geom_now = xcb_get_geometry(conn, window->win);
	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(conn, geom_now, NULL);
	if (!((poin->root_x <= geom->x) || (poin->root_y <= geom->y))) {
		skid[0] = poin->root_x - geom->x - BORDER_WIDTH;
		skid[1] = poin->root_y - geom->y - BORDER_WIDTH;

		if ((skid[0] >= (uint32_t)(WINDOW_MIN_WIDTH)) &&
			(skid[1] >= (uint32_t)(WINDOW_MIN_HEIGHT))) {

			xcb_configure_window(conn, window->win, XCB_CONFIG_WINDOW_WIDTH
				| XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE, skid);
		}
	}
}

void handle_motion_move(window_t *window, xcb_query_pointer_reply_t *poin) {
	if (old_root_x > 0) {
		window->vals[0] += (poin->root_x - old_root_x);
		window->vals[1] += (poin->root_y - old_root_y);
	}

	xcb_configure_window(conn, window->win, XCB_CONFIG_WINDOW_X
		| XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_STACK_MODE, window->vals);
}

void handleButtonPress(xcb_generic_event_t *ev) {
	xcb_button_press_event_t  *e = (xcb_button_press_event_t *) ev;
	win = e->child;
	button = e->detail;

	if (!e->detail || !win)
		return;

	xcb_grab_pointer(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_RELEASE
		| XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
		XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
		screen->root, XCB_NONE, XCB_CURRENT_TIME);

	if (e->detail == MIDDLE_BUTTON) {
		window_t *window = get_window(win);
		handle_motion_tile(window);
	}
}

void handle_motion_notify(xcb_generic_event_t *ev) {

	window_t *window = get_window(win);
	if (!window)
		return;

	window->vals[2] = XCB_STACK_MODE_ABOVE;

	xcb_query_pointer_cookie_t coord = xcb_query_pointer(conn, screen->root);
	xcb_query_pointer_reply_t *poin = xcb_query_pointer_reply(conn, coord, 0);

	if (window->tiling) {
		window = add_window(win, window->vals[0], window->vals[1]);
		remove_window(win);
		adjust_windows();
	}

	switch (button) {
		case LEFT_BUTTON:
			handle_motion_move(window, poin);
			break;

		case RIGHT_BUTTON:
			handle_motion_size(window, poin);
			break;
	}

	old_root_x = poin->root_x;
	old_root_y = poin->root_y;
}

void handleEnterNotify(xcb_generic_event_t *ev) {
	xcb_enter_notify_event_t *e = ( xcb_enter_notify_event_t *) ev;
	printf("Selecting window: %d\n", e->event);
	set_focus(e->event);
}

void handleButtonRelease(xcb_generic_event_t *ev) {
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
	win = 0;
	old_root_x = 0;
	old_root_y = 0;
}

void handle_focus_in(xcb_generic_event_t *ev) {
	xcb_focus_in_event_t *e = (xcb_focus_in_event_t *) ev;
	setFocusColor(e->event, 1);
}

void send_event(xcb_window_t win, xcb_atom_t atom) {

}

void handle_closewindow(xcb_generic_event_t *e) {
	xcb_destroy_notify_event_t *ev = (xcb_destroy_notify_event_t *)e;
	xcb_window_t w_destroy = e ? ev->window : win;

	window_t *w = get_window(w_destroy);

	xcb_intern_atom_cookie_t cookie = xcb_intern_atom (conn, FALSE, 16, "WM_DELETE_WINDOW");
	xcb_intern_atom_reply_t *wm_delete = xcb_intern_atom_reply(conn, cookie, NULL);

	xcb_intern_atom_cookie_t wm_cookie = xcb_intern_atom (conn, FALSE, 12, "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *wm_protocols = xcb_intern_atom_reply(conn, wm_cookie, NULL);

	xcb_client_message_event_t *dev = malloc(32);
    dev->response_type = XCB_CLIENT_MESSAGE;
    dev->window = w_destroy;
    dev->type = wm_protocols->atom;
    dev->format = 32;
    dev->data.data32[0] = wm_delete->atom;
    dev->data.data32[1] = XCB_CURRENT_TIME;

	if (w) {
		xcb_send_event(conn, FALSE, w_destroy, XCB_EVENT_MASK_NO_EVENT, (char *)dev);
		//xcb_destroy_window(conn, w_destroy);
		if (w->tiling) {
			remove_window(w_destroy);
			adjust_windows();
		}
		else {
			del_window(w_destroy);
		}
	}
}

void handle_focus_out(xcb_generic_event_t *ev) {
	xcb_focus_out_event_t *e = (xcb_focus_out_event_t *) ev;
	setFocusColor(e->event, 0);
}

xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode) {
	xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(conn);
	xcb_keysym_t keysym = (!(keysyms) ? 0 : xcb_key_symbols_get_keysym(keysyms, keycode, 0));;
	xcb_key_symbols_free(keysyms);
	return keysym;
}

xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t keysym) {
	xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(conn);
	xcb_keycode_t *keycode = (!(keysyms) ? NULL : xcb_key_symbols_get_keycode(keysyms, keysym));
	xcb_key_symbols_free(keysyms);
	return keycode;
}

void setup(xcb_screen_t *screen) {
	values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE;

	xcb_change_window_attributes_checked(conn, screen->root,
		XCB_CW_EVENT_MASK, values);
	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

	for (int i = 0; i < LEN(keys); ++i) {
		xcb_keycode_t *keycode = xcb_get_keycodes(keys[i].keysym);

		if (keycode) {
			xcb_grab_key(conn, 1, screen->root, keys[i].mod, *keycode,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC );
		}
	}

	xcb_flush(conn);
	xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
		XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE, 1, MOD1);
	xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
		XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE, 2, MOD1);
	xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
		XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE, 3, MOD1);
	xcb_flush(conn);
}

void get_screens(void) {
    xcb_xinerama_query_screens_reply_t *reply;
    xcb_xinerama_screen_info_t *screen_info;

    reply = xcb_xinerama_query_screens_reply(conn, xcb_xinerama_query_screens_unchecked(conn), NULL);

    if (!reply) {
        DIE("Couldn't get Xinerama screens\n");
    }

    screen_info = xcb_xinerama_query_screens_screen_info(reply);
    screen_count = xcb_xinerama_query_screens_screen_info_length(reply);

    for (int i = 0; i < screen_count; i++) {
    	mons[i].x = screen_info[i].x_org;
    	mons[i].y = screen_info[i].y_org;
    }
}

void update_screen(xcb_connection_t *conn) {
	monitor_t *last = NULL;

	xcb_query_pointer_cookie_t coord = xcb_query_pointer(conn, screen->root);
	xcb_query_pointer_reply_t *poin = xcb_query_pointer_reply(conn, coord, 0);

	for (int i = 0; i < screen_count; i++) {
		if (poin->root_x > mons[i].x)
			last = &mons[i];
		else
			break;
	}

	// new screen detected
	if (last != mon) {
		mon = last;
		set_monitor(mon);
	}
}

int main(void) {
	int aaaa;

	conn = xcb_connect(NULL, &aaaa);
	int ret = xcb_connection_has_error(conn);

	if (ret > 0) {
		DIE("xcb_connection has error (%d)\n", ret);
	}

	BOOL closed = FALSE;

	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	get_screens();
	setup(screen);

	xcb_generic_event_t *ev;
	while (ev = xcb_wait_for_event(conn)) {

		update_screen(conn);

		switch (ev->response_type) {
			case XCB_MOTION_NOTIFY:
				handle_motion_notify(ev);
				break;

			case XCB_ENTER_NOTIFY:
				handleEnterNotify(ev);
				break;

			case XCB_BUTTON_PRESS:
				handleButtonPress(ev);
				break;

			case XCB_BUTTON_RELEASE:
				handleButtonRelease(ev);
				break;

			case XCB_MAP_REQUEST:
				handle_map_request(ev);
				break;

			case XCB_FOCUS_IN:
				handle_focus_in(ev);
				break;

			case XCB_FOCUS_OUT:
				handle_focus_out(ev);
				break;

			case XCB_KEY_PRESS:
				handle_key_press(ev);
				break;

			case XCB_DESTROY_NOTIFY:
				handle_closewindow(ev);
				break;
		}

		free(ev);
		xcb_flush(conn);
	}	
}
