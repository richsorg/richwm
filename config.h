#pragma once

#include "keys.h"
#include "tiling.h"
#include "main.h"

void set_tag(arg_t);
void set_workspace(int);

//#define MOD1                   XCB_MOD_MASK_4
#define MOD1 				   XCB_MOD_MASK_1
#define MOD2                   XCB_MOD_MASK_SHIFT

#define MONITORS 				2

#define WINDOW_WIDTH           1910      
#define WINDOW_HEIGHT          1070      
#define WINDOW_MIN_WIDTH       60       
#define WINDOW_MIN_HEIGHT      40 
#define BORDER_WIDTH           1

#define BORDER_COLOR_UNFOCUSED 0x696969 /* 0xRRGGBB */
#define BORDER_COLOR_FOCUSED   0xFFFFFF /* 0xRRGGBB */

#define WINDOW_SIZE WINDOW_WIDTH / 2

#define TAGKEYS(KEY, TAG) \
	{ MOD1,                  KEY,      set_workspace,           .arg={.d = TAG}}, \
	{ MOD1|MOD2,             KEY,      set_tag,           		.arg={.d = TAG}}

static char *tags[] = {"1", "2", "3", "4", "5", "6"};
  
static char *termcmd[] = {"st", NULL};
static char *menucmd[] = {"rofi", "-show", "dun", NULL};
static char *printcmd[] = {"flameshot", "gui", NULL};
static Key keys[] = {
	{ MOD1|MOD2,    XK_Return, spawn,      			.arg={.l = termcmd}},
	{ MOD1,    		XK_p, 	   spawn, 				.arg={.l = menucmd}},
	{ MOD1,    		XK_Print,  spawn, 				.arg={.l = printcmd}},
	TAGKEYS(		XK_1, 							0),
	TAGKEYS(		XK_2, 							1),
	TAGKEYS(		XK_3, 							2),
	TAGKEYS(		XK_4, 							3),
	TAGKEYS(		XK_5, 							4),
	TAGKEYS(		XK_6, 							5),
	TAGKEYS(		XK_7, 							6),
	TAGKEYS(		XK_8, 							7),
	TAGKEYS(		XK_9, 							8),
	{ MOD1,    		XK_q, 	   						handle_closewindow, 	NULL}
};
