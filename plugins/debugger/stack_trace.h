/*
    stack_trace.h
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

#ifndef _STACK_TRACE_H_
#define _STACK_TRACE_H_

#include <glib.h>
#include <gtk/gtkwidget.h>

typedef struct _StackTrace StackTrace;

StackTrace *stack_trace_new (void);

/* Getters */
GtkWidget *stack_trace_get_treeview (StackTrace *st);

void stack_trace_clear (StackTrace *st);
void stack_trace_set_frame (StackTrace *st, gint frame);
void stack_trace_update (GList *lines, gpointer st);
void stack_trace_update_controls (StackTrace *st);
void stack_trace_destroy (StackTrace *st);

#endif
