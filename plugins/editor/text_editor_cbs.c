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

#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>

// #include "anjuta.h"
// #include "controls.h"
#include "text_editor.h"
#include "text_editor_cbs.h"
// #include "text_editor_gui.h"
// #include "mainmenu_callbacks.h"
// #include "dnd.h"
#include "Scintilla.h"

gboolean
on_text_editor_text_buttonpress_event (GtkWidget * widget,
				       GdkEventButton * event,
				       gpointer user_data)
{
	TextEditor *te = user_data;
	text_editor_check_disk_status (te, FALSE);
	gtk_widget_grab_focus (GTK_WIDGET (te->scintilla));
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
	// text_editor_menu_popup (((TextEditor *) user_data)->menu, bevent);
	return TRUE;
}

void
on_text_editor_text_changed (GtkEditable * editable, gpointer user_data)
{
	TextEditor *te = (TextEditor *) user_data;
	if (text_editor_is_saved (te))
	{
		//FIXME:
		// anjuta_update_title ();
		// update_main_menubar ();
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
	if (te->uri == NULL)
		return TRUE;
	if (text_editor_is_saved (te))
		return TRUE;
	if (text_editor_save_file (te, TRUE))
	{
		//FIXME: anjuta_status (_("Autosaved \"%s\""), te->filename);
	}
	else
	{
		GtkWidget *parent;
		parent = gtk_widget_get_toplevel (GTK_WIDGET (te));
		anjuta_util_dialog_warning (GTK_WINDOW (parent),
									_("Autosave \"%s\" -- Failed"), te->filename);
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
		//FIXME:
		// breakpoints_dbase_toggle_singleclick (GPOINTER_TO_INT(line));
	}
	return FALSE;
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
		// FIXME:
		// scintilla_uri_dropped(nt->text);
		break;
	case SCN_SAVEPOINTREACHED:
	case SCN_SAVEPOINTLEFT:
		//FIXME:
		// anjuta_update_page_label(te);
		// anjuta_update_title ();
		// update_main_menubar ();
		text_editor_update_controls (te);
		return;
	case SCN_UPDATEUI:
		//FIXME: anjuta_update_app_status (FALSE, NULL);
		te->current_line = text_editor_get_current_lineno (te);
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
			// FIXME:
			#if 0
			if (!breakpoints_dbase_toggle_doubleclick (line))
			{
				/*  Debugger not active ==> Toggle Bookmarks */
				text_editor_goto_line (te, line, FALSE, FALSE);
				aneditor_command (te->editor_id, ANE_BOOKMARK_TOGGLE, 0, 0);
			}
			#endif
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
