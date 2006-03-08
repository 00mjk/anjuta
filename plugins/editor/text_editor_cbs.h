/* 
    text_editor_cbs.h
    Copyright (C) 2003 Naba Kumar

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
#ifndef _TEXT_EDITOR_CBS_H_
#define _TEXT_EDITOR_CBS_H_

#include <gnome.h>
#include "text_editor.h"

gboolean
on_text_editor_scintilla_focus_in (GtkWidget* scintilla, GdkEvent *event,
								   TextEditor *te);
void
on_text_editor_text_changed            (GtkEditable     *editable,
                                        gpointer         user_data);

gboolean
on_text_editor_text_event              (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_text_editor_insert_text (GtkEditable * text,
							const gchar * insertion_text,
							gint length, gint * pos, TextEditor * te);

gboolean
on_text_editor_text_buttonpress_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_text_editor_scintilla_notify(GtkWidget* sci,	gint wParam,
								gpointer lParam, gpointer data);

void
on_text_editor_scintilla_size_allocate (GtkWidget *widget,
										GtkAllocation *allocation,
										gpointer data);

#endif
