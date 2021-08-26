#pragma once

#include "tiling.h"
#include "wins.h"
#include "main.h"
#include "config.h"
#include "util.h"

struct monitor_t {
    tile_t workspaces[LEN(tags)];

    int x, y;

    xcb_screen_t tmp;
    xcb_screen_t *screen;
} typedef monitor_t;

void set_monitor(monitor_t *);

extern monitor_t mons[MONITORS];
extern monitor_t *mon;
