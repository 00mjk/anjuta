/*
 * text_editor_cbs.c
 * Copyright (C) 2000  Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <ctype.h>

#include "anjuta.h"
#include "controls.h"
#include "text_editor.h"
#include "text_editor_cbs.h"
#include "text_editor_gui.h"
#include "mainmenu_callbacks.h"
#include "resources.h"
#include "dnd.h"
#include "Scintilla.h"

void
on_text_editor_window_realize (GtkWidget * widget, gpointer user_data)
{
	TextEditor *te = user_data;
	te->widgets.window = widget;
}

void
on_text_editor_window_size_allocate (GtkWidget * widget,
				     GtkAllocation * allocation,
				     gpointer user_data)
{
	TextEditor *te = user_data;
	te->geom.x = allocation->x;
	te->geom.y = allocation->y;
	te->geom.width = allocation->width;
	te->geom.height = allocation->height;
}

void
on_text_editor_client_realize (GtkWidget * widget, gpointer user_data)
{
	TextEditor *te = user_data;
	te->widgets.client = widget;
}

gboolean
on_text_editor_text_buttonpress_event (GtkWidget * widget,
				       GdkEventButton * event,
				       gpointer user_data)
{
	TextEditor *te = user_data;
	text_editor_check_disk_status (te, FALSE);
	gtk_widget_grab_focus (GTK_WIDGET (te->widgets.editor));
	return FALSE;
}

gboolean
on_text_editor_text_event (GtkWidget * widget,
			   GdkEvent * event, gpointer user_data)
{
	GdkEventButton *bevent;
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;
	if (((GdkEventButton *) event)->button != 3)
		return FALSE;
	bevent = (GdkEventButton *) event;
	bevent->button = 1;
	text_editor_menu_popup (((TextEditor *) user_data)->menu, bevent);
	return TRUE;
}

gboolean
on_text_editor_window_delete (GtkWidget * widget,
			      GdkEventFocus * event, gpointer user_data)
{
	on_close_file1_activate (NULL, user_data);
	return TRUE;
}

void
on_text_editor_notebook_close_page (GtkNotebook * notebook,
				GtkNotebookPage * page,
				gint page_num, gpointer user_data)
{
	 g_signal_emit_by_name (G_OBJECT
                            (app->widgets.menubar.file.close_file),
                            "activate");
}

gboolean
on_text_editor_window_focus_in_event (GtkWidget * widget,
				      GdkEventFocus * event,
				      gpointer user_data)
{
	TextEditor *te = user_data;
	anjuta_set_current_text_editor (te);
	return FALSE;
}

void
on_text_editor_dock_activate (GtkButton * button, gpointer user_data)
{
	GtkWidget *tab_widget, *eventbox;
	
	TextEditor *te = anjuta_get_current_text_editor ();
	
	tab_widget = text_editor_tab_widget_new(te);

	eventbox = gtk_event_box_new ();
	gtk_widget_show (eventbox);
	text_editor_dock (te, eventbox);

	/* For your kind info, this same data are also set in */
	/* the function anjuta_append_text_editor() */
	g_object_set_data (G_OBJECT (eventbox), "TextEditor", te);

	gtk_notebook_prepend_page (GTK_NOTEBOOK (app->widgets.notebook),
				   eventbox, tab_widget);
	gtk_window_set_title (GTK_WINDOW (app->widgets.window),
			      te->full_filename);
	gtk_notebook_set_menu_label_text(GTK_NOTEBOOK
					(app->widgets.notebook), eventbox,
					te->filename);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (app->widgets.notebook), 0);
	anjuta_update_page_label(te);

	if (anjuta_preferences_get_int (app->preferences, EDITOR_TABS_ORDERING))
		anjuta_order_tabs ();
}

void
on_text_editor_text_changed (GtkEditable * editable, gpointer user_data)
{
	TextEditor *te = (TextEditor *) user_data;
	if (text_editor_is_saved (te))
	{
		anjuta_update_title ();
		update_main_menubar ();
		text_editor_update_controls (te);
	}
}

void
on_text_editor_insert_text (GtkEditable * text,
			    const gchar * insertion_text,
			    gint length, gint * pos, TextEditor * te)
{
}

gboolean on_text_editor_auto_save (gpointer data)
{
	TextEditor *te = data;

	if (!te)
		return FALSE;
	if (anjuta_preferences_get_int (te->preferences, SAVE_AUTOMATIC) == FALSE)
	{
		te->autosave_on = FALSE;
		return FALSE;
	}
	if (te->full_filename == NULL)
		return TRUE;
	if (text_editor_is_saved (te))
		return TRUE;
	if (text_editor_save_file (te, TRUE))
	{
		anjuta_status (_("Autosaved \"%s\""), te->filename);
	}
	else
	{
		anjuta_warning (_("Autosave \"%s\" -- Failed"), te->filename);
	}
	return TRUE;
}


gboolean timerclick = FALSE;

static gboolean
click_timeout (gpointer line)
{	
	/*  If not second clic after timeout : Single Click  */
	if (timerclick)
	{
		timerclick = FALSE;
		breakpoints_dbase_toggle_singleclick (GPOINTER_TO_INT(line));
	}	
	return FALSE;
}

#define IS_DECLARATION(T) ((tm_tag_prototype_t == (T)) || (tm_tag_externvar_t == (T)) \
  || (tm_tag_typedef_t == (T)))

void anjuta_explorer_view_goto_symbol(TextEditor *te, gchar *symbol, gboolean prefer_definition)
{
	GPtrArray *tags;
	TMTag *tag = NULL, *local_tag = NULL, *global_tag = NULL;
	TMTag *local_proto = NULL, *global_proto = NULL;
	guint i;
	int cmp;
	gchar *buf;

	g_return_if_fail(symbol);

	if (!te)
		te = anjuta_get_current_text_editor();

	/* Get the matching definition and declaration in the local file */
	if (te && (te->tm_file) && (te->tm_file->tags_array) &&
		(te->tm_file->tags_array->len > 0))
	{
		for (i=0; i < te->tm_file->tags_array->len; ++i)
		{
			tag = TM_TAG(te->tm_file->tags_array->pdata[i]);
			cmp =  strcmp(symbol, tag->name);
			if (0 == cmp)
			{
				if (IS_DECLARATION(tag->type))
					local_proto = tag;
				else
					local_tag = tag;
			}
			else if (cmp < 0)
				break;
		}
	}
	if (!(((prefer_definition) && (local_tag)) || ((!prefer_definition) && (local_proto))))
	{
		/* Get the matching definition and declaration in the workspace */
		tags =  TM_WORK_OBJECT(tm_get_workspace())->tags_array;
		if (tags && (tags->len > 0))
		{
			for (i=0; i < tags->len; ++i)
			{
				tag = TM_TAG(tags->pdata[i]);
				if (tag->atts.entry.file)
				{
					cmp = strcmp(symbol, tag->name);
					if (cmp == 0)
					{
						if (IS_DECLARATION(tag->type))
							global_proto = tag;
						else
							global_tag = tag;
					}
					else if (cmp < 0)
						break;
				}
			}
		}
	}
	if (prefer_definition)
	{
		if (local_tag)
			tag = local_tag;
		else if (global_tag)
			tag = global_tag;
		else if (local_proto)
			tag = local_proto;
		else
			tag = global_proto;
	}
	else
	{
		if (local_proto)
			tag = local_proto;
		else if (global_proto)
			tag = global_proto;
		else if (local_tag)
			tag = local_tag;
		else
			tag = global_tag;
	}

	if (tag)
	{
		anjuta_explorer_view_goto_file_line_mark(tag->atts.entry.file->work_object.file_name
		  , tag->atts.entry.line, TRUE);
	
		/* hack, when a struct is define as "typedef struct _a a", 
		   we would like to view _a but not just a typedef line.;) */
		if ( '_' != *symbol)
		{
			buf = g_strdup_printf("_%s", symbol);
			anjuta_explorer_view_goto_symbol(te, buf, TRUE);
			g_free(buf);
		}
	}
}

void anjuta_explorer_current_keyword(void)
{
		TextEditor *te;
	    gchar *buf = NULL;
        static gchar *symb_saved = NULL;
 
        te = anjuta_get_current_text_editor();
        if(!te) return;

		/* no project/ no tags etc, we could do nothing.:) */
		if (!app || !app->project_dbase || !app->project_dbase->project_is_open
		|| !app->project_dbase->tm_project ||
	    !app->project_dbase->tm_project->tags_array ||
	    (0 == app->project_dbase->tm_project->tags_array->len))
			return;		
		
        buf = text_editor_get_current_word(te);
        if ((buf == NULL) || ('\0' == *buf )) 	return;
		
		if (symb_saved && strcmp(buf, symb_saved) == 0)
			return;

		anjuta_explorer_view_goto_symbol(te, buf, TRUE);

		g_free(symb_saved);
		symb_saved = buf;
}

void
on_text_editor_scintilla_notify (GtkWidget * sci,
				 gint wParam, gpointer lParam, gpointer data)
{
	TextEditor *te;
	struct SCNotification *nt;
	gint line;
	
	te = data;
	if (te->freeze_count != 0)
		return;
	nt = lParam;
	switch (nt->nmhdr.code)
	{
	case SCN_URIDROPPED:
		scintilla_uri_dropped(nt->text);
		break;
	case SCN_SAVEPOINTREACHED:
	case SCN_SAVEPOINTLEFT:
		anjuta_update_page_label(te);
		anjuta_update_title ();
		update_main_menubar ();
		text_editor_update_controls (te);
		return;
	case SCN_UPDATEUI:
		anjuta_update_app_status (FALSE, NULL);
		te->current_line = text_editor_get_current_lineno (te);
	    anjuta_explorer_current_keyword();
		return;
		
	case SCN_CHARADDED:
		te->current_line = text_editor_get_current_lineno (te);
		text_editor_set_indicator (te, te->current_line, -1);
		return;

	case SCN_MARGINCLICK:	
	line =	text_editor_get_line_from_position (te, nt->position);
	if (nt->margin == 1)  /*  Bookmarks and Breakpoints  margin */
	{
		/* if second click before timeout : Double click  */
		if (timerclick)
		{
			timerclick = FALSE;
			/* Toggle Breakpoints */
			if (! breakpoints_dbase_toggle_doubleclick (line))
			{
				/*  Debugger not active ==> Toggle Bookmarks */
				text_editor_goto_line (te, line, FALSE, FALSE);
				aneditor_command (te->editor_id, ANE_BOOKMARK_TOGGLE, 0, 0);
			}
		}
		else
		{
			timerclick = TRUE;
			/* Timeout after 400ms  */
			/* If 2 clicks before the timeout ==> Single Click */
			g_timeout_add (400, (void*) click_timeout, GINT_TO_POINTER(line));
		}
	}
	return;
	
/*	case SCEN_SETFOCUS:
	case SCEN_KILLFOCUS:
	case SCN_STYLENEEDED:
	case SCN_DOUBLECLICK:
	case SCN_MODIFIED:
	case SCN_NEEDSHOWN:
*/
	default:
		return;
	}
}

void
on_text_editor_scintilla_size_allocate (GtkWidget * widget,
					GtkAllocation * allocation,
					gpointer data)
{
	TextEditor *te;
	te = data;
	g_return_if_fail (te != NULL);

	if (te->first_time_expose == FALSE)
		return;

	te->first_time_expose = FALSE;
	text_editor_goto_line (te, te->current_line, FALSE, FALSE);
}
