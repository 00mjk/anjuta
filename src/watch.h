/*
    watch.h
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

#ifndef _WATCH_H_
#define _WATCH_H_

#include <gnome.h>
#include "properties.h"

typedef struct _ExprWatchGui ExprWatchGui;
typedef struct _ExprWatch ExprWatch;

struct _ExprWatchGui
{
    GtkWidget*   window;
    GtkWidget*   clist;
    GtkWidget*   menu_add;
    GtkWidget*   menu_remove;
    GtkWidget*   menu_clear;
    GtkWidget*   menu_update;
    GtkWidget*   menu_toggle;
    GtkWidget*   menu_change;
    GtkWidget*   menu;
};

struct _ExprWatch
{
  ExprWatchGui  widgets;
  GList                    *exprs;
  gint                      current_index;
  gint                      count;
  gboolean             is_showing;
  gint                     win_pos_x, win_pos_y;
  gint                     win_width, win_height;
};

ExprWatch*
expr_watch_new(void);

GtkWidget*
create_watch_menu(void);

void
create_expr_watch_gui(ExprWatch* ew);

GtkWidget*
create_watch_add_dialog(void);

GtkWidget*
create_watch_change_dialog(void);

GtkWidget*
create_eval_dialog(void);

void
expr_watch_clear(ExprWatch *ew);

void
expr_watch_cmd_queqe(ExprWatch *ew);

void
expr_watch_update(GList *lines, gpointer  ew);

void
expr_watch_update_controls(ExprWatch* ew);

void
expr_watch_destroy(ExprWatch*ew);

gboolean
expr_watch_save_yourself(ExprWatch* ew, FILE* stream);

gboolean
expr_watch_load_yourself(ExprWatch* ew, PropsID props);

void
expr_watch_show(ExprWatch * ew);

void
expr_watch_hide(ExprWatch * ew);

void
eval_output_arrived(GList *outputs, gpointer data);

#endif

