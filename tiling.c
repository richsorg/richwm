#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "monitor.h"
#include "bool.h"
#include "main.h"
#include "tiling.h"
#include "config.h"

tile_t *tile = &mons[0].workspaces[0];

void set_workspace(int tag) {
	if (tile == &mon->workspaces[tag])
		return;

	xcb_unmap_window(conn, tile->master.win);

	for (int i = 0; i < tile->count; i++) {
		if (tile->wins[i].win)
			xcb_unmap_window(conn, tile->wins[i].win);
	}

	tile = &mon->workspaces[tag];

	xcb_map_window(conn, tile->master.win);
	for (int i = 0; i < tile->count; i++) {
		if (tile->wins[i].win)
			xcb_map_window(conn, tile->wins[i].win);
	}
}

void set_tag(arg_t arg) {
	int tag = arg.d;
	xcb_map_request_event_t e = {.window = win};

	tile_t *old = tile;
	tile = &mon->workspaces[tag];

	handle_map_request((void *)&e);
	xcb_unmap_window(conn, win);

	tile = old;
	remove_window(win);
}

void resize_window(window_t *window) {
	window->vals[5] = XCB_STACK_MODE_BELOW;

	xcb_configure_window(conn, window->win, XCB_CONFIG_WINDOW_X |
		XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
		XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH |
		XCB_CONFIG_WINDOW_STACK_MODE, window->vals);

	//xcb_flush(conn);

	values[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
	xcb_change_window_attributes_checked(conn, window->win,
		XCB_CW_EVENT_MASK, values);
	set_focus(window->win);
}

void resize_master(int idx) {
	if (idx == 0) {
		MASTER_WINDOW((&tile->master));
	}
	else {
		tile->master.vals[2] = WINDOW_WIDTH / 2;
	}
}

void resize_windows(int idx) {
	resize_master(idx);
	resize_window(&tile->master);

	for (int i = 0; i <= idx; i++) {
		if (i > 0) {
			window_t *new = &tile->wins[i];

			STACK_NEXT((new), (&tile->wins[i - 1]));
			resize_window(new);
		}
		else if (idx) {
			window_t *new = &tile->wins[i];
			STACK_START(new, idx);
			resize_window(new);
		}
	}
}

void handle_map_request(xcb_generic_event_t *ev) {
	xcb_map_request_event_t *e = (xcb_map_request_event_t *) ev;
	int idx = get_last_window();

	if (idx >= MAX_WINDOWS - 1)
		return;

	printf("making window: %d\n", e->window);
	xcb_map_window(conn, e->window);

	if (idx == -1) {
		MASTER_WINDOW((&tile->master));
		tile->master.win = e->window;
		resize_window(&tile->master);
	}
	else {
		SLAVE_WINDOW((&tile->wins[idx]));
		tile->wins[idx].win = e->window;
		resize_windows(tile->count);
	}

	tile->count++;
}

window_t *get_window(xcb_window_t window) {
	if (!window)
		return NULL;

	if (tile->master.win == window)
		return &tile->master;

	for (int i = 0; i <= tile->count; i++) {
		if (tile->wins[i].win == window)
			return &tile->wins[i];
	}

	window_t *tmp = window_head;

	while (tmp) {
		if (tmp->win == window)
			return tmp;
		tmp = tmp->next;
	}

	return NULL;
}

void remove_window(xcb_window_t window) {

	if (tile->master.win == window) {
		tile->master = CLEAN_WINDOW();
		--tile->count;
	}
	else {
		for (int i = 0; i <= tile->count; i++) {
			if (tile->wins[i].win == window) {
				tile->wins[i] = CLEAN_WINDOW();
				--tile->count;
				break;
			}
		}
	}
}

void adjust_windows(void) {
	int idx = get_last_window();

	if (idx == -1) {
		tile->master = tile->wins[0];
		tile->wins[0] = CLEAN_WINDOW();
		MASTER_WINDOW((&tile->master));
		idx = 0;
	}

	SHUFFLE_STACK(tile->wins, tile->count, idx);
	resize_windows(tile->count - 1);
}

int get_last_window(void) {
	if (!tile->master.win)
		return -1;

	for (int i = 0; i < MAX_WINDOWS; i++) {
		if (!tile->wins[i].win)
			return i;
	}

	return MAX_WINDOWS;
}
