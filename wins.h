#pragma once

#include "bool.h"

#define MAX_WINDOWS 256

struct window_t {
	xcb_window_t win;
	uint32_t vals[6];
	BOOL tiling;

	struct window_t *next;
} typedef window_t;

window_t *add_window(xcb_window_t, uint32_t, uint32_t);
window_t *get_window(xcb_window_t);
extern window_t *window_head;
