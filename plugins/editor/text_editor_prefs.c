/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    prefs.c
    Copyright (C) 2000 Naba Kumar

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
#include <libanjuta/anjuta-preferences.h>
#include "properties.h"
#include "text_editor_prefs.h"
#include "text_editor_cbs.h"

#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"

#include "lexer.h"
#include "aneditor.h"

/* Editor preferences notifications */

#if 0

typedef {
	TextEditor *te;
	gchar *key;
	GCallback proxy;
	gpointer proxy_data;
	gboolean is_string;
} PrefPassedData;

static PrefPassedData*
pref_passed_data_new (TextEditor *te, const gchar *key, gboolean is_string,
					  GCallback *proxy, gponter proxy_data)
{
	PrefPassedData *pd = g_new0(PrefPassedData, 1);
	pd->te = te;
	pd->key = g_strdup (key);
	pd->proxy = proxy;
	pd->is_string = is_string;
	pd->data1 = data1;
	pd->data2 = data2;
	pd->data3 = data3;
	pd->data4 = data4;
}

static void
pref_gconf_notify (GConfClient *gclient, guint cnxn_id,
				   GConfEntry *entry, gpointer user_data)
{
	PrefPassedData *pd = (PrefPassedData*)user_data;
	if (!is_string)
		set_n_get_prop_int (pd->te, pd->key);
	else
		set_n_get_prop_string (pd->te, pd->key);
	if (pd->proxy)
		(pd->proxy)(data1, data2, data3, data4);
}

#endif

static gint
set_n_get_prop_int (TextEditor *te, const gchar *key)
{
	gint val;
	AnjutaPreferences *pr;
	pr = te->preferences;
	val = anjuta_preferences_get_int (pr, key);
	prop_set_int_with_key (text_editor_get_props (), key, val);
	return val;
}

static void
on_gconf_notify_timer (GConfClient *gclient, guint cnxn_id,
					   GConfEntry *entry, gpointer user_data)
{
	gboolean auto_save;
	guint auto_save_timer;
	TextEditor *te;
	
	te = TEXT_EDITOR (user_data);
	auto_save = set_n_get_prop_int (te, SAVE_AUTOMATIC);
	auto_save_timer = set_n_get_prop_int (te, AUTOSAVE_TIMER);

	if (auto_save)
	{
		if (te->autosave_on == TRUE)
		{
			if (auto_save_timer != te->autosave_it)
			{
				gtk_timeout_remove (te->autosave_id);
				te->autosave_id =
					gtk_timeout_add (auto_save_timer *
							 60000,
							 on_text_editor_auto_save,
							 te);
			}
		}
		else
		{
			te->autosave_id =
				gtk_timeout_add (auto_save_timer * 60000,
						 on_text_editor_auto_save,
						 te);
		}
		te->autosave_it = auto_save_timer;
		te->autosave_on = TRUE;
	}
	else
	{
		if (te->autosave_on == TRUE)
			gtk_timeout_remove (te->autosave_id);
		te->autosave_on = FALSE;
	}
}

static void
on_gconf_notify_disable_hilite (GConfClient *gclient, guint cnxn_id,
								GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	
	te = TEXT_EDITOR (user_data);
	set_n_get_prop_int (te, DISABLE_SYNTAX_HILIGHTING);
	text_editor_set_hilite_type (te);
}

static void
on_gconf_notify_zoom_factor (GConfClient *gclient, guint cnxn_id,
							 GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gint zoom_factor;
	
	te = TEXT_EDITOR (user_data);
	zoom_factor = set_n_get_prop_int (te, TEXT_ZOOM_FACTOR);
	text_editor_set_zoom_factor (te, zoom_factor);
}

static void
on_gconf_notify_tab_size (GConfClient *gclient, guint cnxn_id,
						  GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gint tab_size;
	
	te = TEXT_EDITOR (user_data);
	tab_size = set_n_get_prop_int (te, TAB_SIZE);
	text_editor_command (te, ANE_SETTABSIZE, tab_size, 0);
}

static void
on_gconf_notify_use_tab_for_indentation (GConfClient *gclient, guint cnxn_id,
										 GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean use_tabs;
	
	te = TEXT_EDITOR (user_data);
	use_tabs = set_n_get_prop_int (te, USE_TABS);
	text_editor_scintilla_command (te, SCI_SETTABWIDTH,	use_tabs, 0);
}

static void
on_gconf_notify_automatic_indentation (GConfClient *gclient, guint cnxn_id,
									   GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean indent_automatic;
	
	te = TEXT_EDITOR (user_data);
	indent_automatic = set_n_get_prop_int (te, INDENT_AUTOMATIC);
}

static void
on_gconf_notify_indent_size (GConfClient *gclient, guint cnxn_id,
							 GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gint indent_size;
	
	te = TEXT_EDITOR (user_data);
	indent_size = set_n_get_prop_int (te, INDENT_SIZE);
	text_editor_scintilla_command (te, SCI_SETINDENT, indent_size, 0);
}

static void
on_gconf_notify_wrap_bookmarks (GConfClient *gclient, guint cnxn_id,
								GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	
	te = TEXT_EDITOR (user_data);
	set_n_get_prop_int (te, WRAP_BOOKMARKS);
}

static void
on_gconf_notify_braces_check (GConfClient *gclient, guint cnxn_id,
							  GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	
	te = TEXT_EDITOR (user_data);
	set_n_get_prop_int (te, BRACES_CHECK);
}

static void
on_gconf_notify_indent_opening (GConfClient *gclient, guint cnxn_id,
								GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	
	te = TEXT_EDITOR (user_data);
	set_n_get_prop_int (te, INDENT_OPENING);
}

static void
on_gconf_notify_indent_closing (GConfClient *gclient, guint cnxn_id,
								GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	
	te = TEXT_EDITOR (user_data);
	set_n_get_prop_int (te, INDENT_CLOSING);
}

static void
on_gconf_notify_margin_linenum_width (GConfClient *gclient, guint cnxn_id,
									  GConfEntry *entry, gpointer user_data)
{
#if 0
	TextEditor *te;
	gint width;
	
	te = TEXT_EDITOR (user_data);
	width = set_n_get_prop_int (te, MARGIN_LINENUMBER_WIDTH);
	text_editor_scintilla_command (te, SCI_SETMARGINWIDTHN,	0, width);
#endif
}

static void
on_gconf_notify_view_eols (GConfClient *gclient, guint cnxn_id,
							   GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean state;
	
	te = TEXT_EDITOR (user_data);
	state = set_n_get_prop_int (te, VIEW_EOL);
	text_editor_command (te, ANE_VIEWEOL, state, 0);
}

static void
on_gconf_notify_view_whitespaces (GConfClient *gclient, guint cnxn_id,
								  GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean state;
	
	te = TEXT_EDITOR (user_data);
	state = set_n_get_prop_int (te, VIEW_WHITE_SPACES);
	text_editor_command (te, ANE_VIEWSPACE, state, 0);
}

static void
on_gconf_notify_view_linewrap (GConfClient *gclient, guint cnxn_id,
						  GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean state;
	
	te = TEXT_EDITOR (user_data);
	state = set_n_get_prop_int (te, VIEW_LINE_WRAP);
	text_editor_command (te, ANE_LINEWRAP, state, 0);
}

static void
on_gconf_notify_view_indentation_guides (GConfClient *gclient, guint cnxn_id,
										 GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean state;
	
	te = TEXT_EDITOR (user_data);
	state = set_n_get_prop_int (te, VIEW_INDENTATION_GUIDES);
	text_editor_command (te, ANE_VIEWGUIDES, state, 0);
}

static void
on_gconf_notify_view_folds (GConfClient *gclient, guint cnxn_id,
							GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean state;
	
	te = TEXT_EDITOR (user_data);
	state = set_n_get_prop_int (te, VIEW_FOLD_MARGIN);
	text_editor_command (te, ANE_FOLDMARGIN, state, 0);
}

static void
on_gconf_notify_view_markers (GConfClient *gclient, guint cnxn_id,
							  GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean state;
	
	te = TEXT_EDITOR (user_data);
	state = set_n_get_prop_int (te, VIEW_MARKER_MARGIN);
	text_editor_command (te, ANE_SELMARGIN, state, 0);
}

static void
on_gconf_notify_view_linenums (GConfClient *gclient, guint cnxn_id,
							   GConfEntry *entry, gpointer user_data)
{
	TextEditor *te;
	gboolean state;
	
	te = TEXT_EDITOR (user_data);
	state = set_n_get_prop_int (te, VIEW_LINENUMBERS_MARGIN);
	text_editor_command (te, ANE_LINENUMBERMARGIN, state, 0);
	text_editor_set_line_number_width (te);
}

#define REGISTER_NOTIFY(key, func) \
	notify_id = anjuta_preferences_notify_add (te->preferences, \
											   key, func, te, NULL); \
	te->gconf_notify_ids = g_list_prepend (te->gconf_notify_ids, \
										   (gpointer)(notify_id));

void
text_editor_prefs_init (TextEditor *te)
{
	guint notify_id;
	
	/* Sync prefs from gconf to props */
	set_n_get_prop_int (te, TAB_SIZE);
	set_n_get_prop_int (te, SAVE_AUTOMATIC);
	set_n_get_prop_int (te, AUTOSAVE_TIMER);
	set_n_get_prop_int (te, TEXT_ZOOM_FACTOR);
	set_n_get_prop_int (te, INDENT_SIZE);
	set_n_get_prop_int (te, USE_TABS);
	set_n_get_prop_int (te, DISABLE_SYNTAX_HILIGHTING);
	set_n_get_prop_int (te, INDENT_AUTOMATIC);
	set_n_get_prop_int (te, WRAP_BOOKMARKS);
	set_n_get_prop_int (te, BRACES_CHECK);
	set_n_get_prop_int (te, INDENT_OPENING);
	set_n_get_prop_int (te, INDENT_CLOSING);
	set_n_get_prop_int (te, MARGIN_LINENUMBER_WIDTH);
	set_n_get_prop_int (te, VIEW_EOL);
	set_n_get_prop_int (te, VIEW_LINE_WRAP);
	set_n_get_prop_int (te, VIEW_WHITE_SPACES);
	set_n_get_prop_int (te, VIEW_INDENTATION_GUIDES);
	set_n_get_prop_int (te, VIEW_FOLD_MARGIN);
	set_n_get_prop_int (te, VIEW_MARKER_MARGIN);
	set_n_get_prop_int (te, VIEW_LINENUMBERS_MARGIN);
	
	/* Register gconf notifications */
	REGISTER_NOTIFY (TAB_SIZE, on_gconf_notify_tab_size);
	REGISTER_NOTIFY (SAVE_AUTOMATIC, on_gconf_notify_timer);
	REGISTER_NOTIFY (AUTOSAVE_TIMER, on_gconf_notify_timer);
	REGISTER_NOTIFY (TEXT_ZOOM_FACTOR, on_gconf_notify_zoom_factor);
	REGISTER_NOTIFY (INDENT_SIZE, on_gconf_notify_indent_size);
	REGISTER_NOTIFY (USE_TABS, on_gconf_notify_use_tab_for_indentation);
	REGISTER_NOTIFY (DISABLE_SYNTAX_HILIGHTING, on_gconf_notify_disable_hilite);
	REGISTER_NOTIFY (INDENT_AUTOMATIC, on_gconf_notify_automatic_indentation);
	REGISTER_NOTIFY (WRAP_BOOKMARKS, on_gconf_notify_wrap_bookmarks);
	REGISTER_NOTIFY (BRACES_CHECK, on_gconf_notify_braces_check);
	REGISTER_NOTIFY (INDENT_OPENING, on_gconf_notify_indent_opening);
	REGISTER_NOTIFY (INDENT_CLOSING, on_gconf_notify_indent_closing);
	REGISTER_NOTIFY (MARGIN_LINENUMBER_WIDTH, on_gconf_notify_margin_linenum_width);
	REGISTER_NOTIFY (VIEW_EOL, on_gconf_notify_view_eols);
	REGISTER_NOTIFY (VIEW_LINE_WRAP, on_gconf_notify_view_linewrap);
	REGISTER_NOTIFY (VIEW_WHITE_SPACES, on_gconf_notify_view_whitespaces);
	REGISTER_NOTIFY (VIEW_INDENTATION_GUIDES, on_gconf_notify_view_indentation_guides);
	REGISTER_NOTIFY (VIEW_FOLD_MARGIN, on_gconf_notify_view_folds);
	REGISTER_NOTIFY (VIEW_MARKER_MARGIN, on_gconf_notify_view_markers);
	REGISTER_NOTIFY (VIEW_LINENUMBERS_MARGIN, on_gconf_notify_view_linenums);
	
	/* templates..
	REGISTER_NOTIFY ();
	REGISTER_NOTIFY ();
	REGISTER_NOTIFY ();
	*/
}

void
text_editor_prefs_finalize (TextEditor *te)
{
	GList *node;
	node = te->gconf_notify_ids;
	while (node)
	{
		anjuta_preferences_notify_remove (te->preferences, (guint)node->data);
		node = g_list_next (node);
	}
	g_list_free (te->gconf_notify_ids);
	te->gconf_notify_ids = NULL;
}
