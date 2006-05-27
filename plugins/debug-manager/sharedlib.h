/*
    sharedlibs.h
    Copyright (C) 2000  Kh. Naba Kumar Singh

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _SHAREDLIBS_H_
#define _SHAREDLIBS_H_

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>

#include <glib.h>
#include <gtk/gtkwidget.h>

typedef struct _SharedlibsGui SharedlibsGui;
typedef struct _Sharedlibs Sharedlibs;

struct _SharedlibsGui
{
    GtkWidget*   window;
    GtkWidget*   clist;
    GtkWidget*   menu;
    GtkWidget*   menu_update;
};

struct _Sharedlibs
{
	SharedlibsGui  widgets;
	IAnjutaDebugger *debugger;
	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;
};

Sharedlibs*
sharedlibs_new (IAnjutaDebugger *debugger);

void
sharedlibs_clear (Sharedlibs *ew);

void
sharedlibs_free (Sharedlibs*ew);

void
sharedlibs_show (Sharedlibs * ew);

void
sharedlibs_hide (Sharedlibs * ew);

#endif
