#pragma once

#include "util.h"
#include "wins.h"
#include "monitor.h"
#include "bool.h"
#include "config.h"

struct tile_t {
	window_t master;
	window_t wins[MAX_WINDOWS];
	int count;
} typedef tile_t;

extern tile_t *tile;

#define SHUFFLE_STACK(wins, count, idx)				\
	for (int i = idx; i < count; i++)				\
		wins[i] = wins[i + 1];						\

#define STACK_NEXT(new, old)						\
	new->vals[1] = old->vals[1] + old->vals[3];		\
	new->vals[2] = WINDOW_WIDTH / 2;				\
	new->vals[3] = old->vals[3];					\

#define STACK_START(w, idx)							\
	w->vals[1] = 0;									\
	w->vals[2] = WINDOW_WIDTH / 2;					\
	w->vals[3] = WINDOW_HEIGHT / (idx);				\

#define MASTER_WINDOW(w) MAKE_WINDOW(w, mon->x, 0)
#define SLAVE_WINDOW(w) MAKE_WINDOW(w, mon->x + WINDOW_SIZE, 0)

#define CLEAN_WINDOW() (window_t){0}

#define MAKE_WINDOW(w, width, height)	\
	w->vals[0] = width;					\
	w->vals[1] = height;				\
	w->vals[2] = WINDOW_WIDTH; 			\
	w->vals[3] = WINDOW_HEIGHT; 		\
	w->vals[4] = BORDER_WIDTH; 			\
	w->vals[5] = XCB_STACK_MODE_BELOW; 	\
	w->tiling = TRUE;

window_t *get_window(xcb_window_t);
void remove_window(xcb_window_t);
