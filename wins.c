#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "monitor.h"
#include "bool.h"
#include "wins.h"
#include "tiling.h"
#include "config.h"

int window_count = 0;
window_t *window_head = NULL;

void set_monitor(monitor_t *mon) {
	tile = &mon->workspaces[0];
}

void del_window(xcb_window_t window) {
	window_t *tmp = window_head;

	if (tmp->win == window) {
		tmp = tmp->next;
		return;
	}

	while (tmp->next) {
		if (tmp->win == window)
			break;
		tmp = tmp->next;
	}

	if (tmp) {
		window_t *reap = tmp->next;
		tmp->next = tmp->next->next;
		free(reap);
	}
}

window_t *add_window(xcb_window_t window, uint32_t x, uint32_t y) {
	window_t *win = malloc(1 * sizeof(window_t));
	win->win = window;
	win->vals[0] = x;
	win->vals[1] = y;

	win->next = window_head;
	window_head = win;
	return win;
}
