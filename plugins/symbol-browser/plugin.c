/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) Naba Kumar  <naba@gnome.org>

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

#include <config.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/interfaces/ianjuta-file-manager.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-loader.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-markable.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/plugins.h>
#include <libegg/menu/egg-combo-action.h>

#include <tm_tagmanager.h>
#include "an_symbol_view.h"
#include "an_symbol_search.h"
#include "an_symbol_info.h"
#include "an_symbol_prefs.h"
#include "an_symbol_iter.h"
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-symbol-browser-plugin.ui"
#define PREFS_GLADE PACKAGE_DATA_DIR"/glade/anjuta-symbol-browser-plugin.glade"
#define ICON_FILE "anjuta-symbol-browser-plugin.png"

#define TIMEOUT_INTERVAL_SYMBOLS_UPDATE		10000

static gpointer parent_class = NULL;
static gboolean need_symbols_update;
static gint timeout_id;
static gchar prev_char_added = ' ';

/* these will block signals on treeview and treesearch callbacks functions */
static void trees_signals_block (SymbolBrowserPlugin *sv_plugin);
static void trees_signals_unblock (SymbolBrowserPlugin *sv_plugin);

static void on_treesearch_symbol_selected_event (AnjutaSymbolSearch *search, 
												 AnjutaSymbolInfo *sym, 
												 SymbolBrowserPlugin *sv_plugin);
static void update_editor_symbol_model (SymbolBrowserPlugin *sv_plugin);

#define REGISTER_ICON(icon, stock_id) \
	pixbuf = gdk_pixbuf_new_from_file (icon, NULL); \
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf); \
	gtk_icon_factory_add (icon_factory, stock_id, icon_set); \
	g_object_unref (pixbuf);

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GdkPixbuf *pixbuf;
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	icon_factory = anjuta_ui_get_icon_factory (ui);
	REGISTER_ICON (PACKAGE_PIXMAPS_DIR"/"ICON_FILE, "symbol-browser-plugin-icon");
}

static void
goto_file_line (AnjutaPlugin *plugin, const gchar *filename, gint lineno)
{
	gchar *uri;
	IAnjutaFileLoader *loader;
	
	g_return_if_fail (filename != NULL);
		
	/* Go to file and line number */
	loader = anjuta_shell_get_interface (plugin->shell, IAnjutaFileLoader,
										 NULL);
		
	uri = g_strdup_printf ("file:///%s#%d", filename, lineno);
	ianjuta_file_loader_load (loader, uri, FALSE, NULL);
	g_free (uri);
}

static void
goto_file_tag (SymbolBrowserPlugin *sv_plugin, const char *symbol,
			   gboolean prefer_definition)
{
	const gchar *file;
	gint line;
	gboolean ret;
	
	ret = anjuta_symbol_view_get_file_symbol (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
											  symbol, prefer_definition,
											  &file, &line);
	if (ret)
	{
		goto_file_line (ANJUTA_PLUGIN (sv_plugin), file, line);
	}
}

static void
on_goto_def_activate (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	gchar *file;
	gint line;
	gboolean ret;
	
	ret = anjuta_symbol_view_get_current_symbol_def (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
													 &file, &line);
	if (ret)
	{
		goto_file_line (ANJUTA_PLUGIN (sv_plugin), file, line);
		g_free (file);
	}
}

static void
on_goto_decl_activate (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	gchar *file;
	gint line;
	gboolean ret;
	
	ret = anjuta_symbol_view_get_current_symbol_decl (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
													  &file, &line);
	if (ret)
	{
		goto_file_line (ANJUTA_PLUGIN (sv_plugin), file, line);
		g_free (file);
	}
}

static void
on_goto_file_tag_decl_activate (GtkAction * action,
								SymbolBrowserPlugin *sv_plugin)
{
	IAnjutaEditor *ed;
	gchar *word;

	if (sv_plugin->current_editor)
	{
		ed = IANJUTA_EDITOR (sv_plugin->current_editor);
		word = ianjuta_editor_get_current_word (ed, NULL);
		if (word)
		{
			goto_file_tag (sv_plugin, word, FALSE);
			g_free (word);
		}
	}
}

static void
on_goto_file_tag_def_activate (GtkAction * action,
								SymbolBrowserPlugin *sv_plugin)
{
	IAnjutaEditor *ed;
	gchar *word;

	if (sv_plugin->current_editor)
	{
		ed = IANJUTA_EDITOR (sv_plugin->current_editor);
		word = ianjuta_editor_get_current_word (ed, NULL);
		if (word)
		{
			goto_file_tag (sv_plugin, word, TRUE);
			g_free (word);
		}
	}
}

static void
on_find_activate (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	gchar *symbol;
	symbol = anjuta_symbol_view_get_current_symbol (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree));
	if (symbol)
	{
		g_warning ("TODO: Unimplemented");
		g_free (symbol);
	}
}

static gboolean
on_refresh_idle (gpointer user_data)
{
	IAnjutaProjectManager *pm;
	GList *source_uris;
	GList *source_files;
	AnjutaStatus *status;
	SymbolBrowserPlugin *sv_plugin = (SymbolBrowserPlugin *)user_data;
	
	/* FIXME: There should be a way to ensure that this project manager
	 * is indeed the one that has opened the project_uri
	 */
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sv_plugin)->shell,
									 IAnjutaProjectManager, NULL);
	g_return_val_if_fail (pm != NULL, FALSE);
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (sv_plugin)->shell, NULL);
	anjuta_status_push (status, "Refreshing symbol tree...");
	anjuta_status_busy_push (status);
	
	source_uris = source_files = NULL;
	source_uris = ianjuta_project_manager_get_elements (pm,
										IANJUTA_PROJECT_MANAGER_SOURCE,
										NULL);
	if (source_uris)
	{
		const gchar *uri;
		GList *node;
		node = source_uris;
		
		while (node)
		{
			gchar *file_path;
			
			uri = (const gchar *)node->data;
			file_path = gnome_vfs_get_local_path_from_uri (uri);
			if (file_path)
				source_files = g_list_prepend (source_files, file_path);
			node = g_list_next (node);
		}
		source_files = g_list_reverse (source_files);
	}
	anjuta_symbol_view_update (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
							   source_files);
	g_list_foreach (source_files, (GFunc)g_free, NULL);
	g_list_foreach (source_uris, (GFunc)g_free, NULL);
	g_list_free (source_files);
	g_list_free (source_uris);
	
	/* Current editor symbol model may have changed */
	update_editor_symbol_model (sv_plugin);
	
	anjuta_status_busy_pop (status);
	anjuta_status_pop (status);
	return FALSE;
}

static void
on_refresh_activate (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	if (!sv_plugin->project_root_uri)
		return;
	g_idle_add (on_refresh_idle, sv_plugin);
}

static GtkActionEntry actions[] = 
{
	{ "ActionMenuGoto", NULL, N_("_Goto"), NULL, NULL, NULL},
	{
		"ActionSymbolBrowserGotoDef",
		NULL,
		N_("Tag _Definition"),
		"<control>d",
		N_("Goto symbol definition"),
		G_CALLBACK (on_goto_file_tag_def_activate)
	},
	{
		"ActionSymbolBrowserGotoDecl",
		NULL,
		N_("Tag De_claration"),
		"<shift><control>d",
		N_("Goto symbol declaration"),
		G_CALLBACK (on_goto_file_tag_decl_activate)
	}
};

static GtkActionEntry popup_actions[] = 
{
	{
		"ActionPopupSymbolBrowserGotoDef",
		NULL,
		N_("Goto _Definition"),
		NULL,
		N_("Goto symbol definition"),
		G_CALLBACK (on_goto_def_activate)
	},
	{
		"ActionPopupSymbolBrowserGotoDecl",
		NULL,
		N_("Goto De_claration"),
		NULL,
		N_("Goto symbol declaration"),
		G_CALLBACK (on_goto_decl_activate)
	},
	{
		"ActionPopupSymbolBrowserFind",
		GTK_STOCK_FIND,
		N_("_Find Usage"),
		NULL,
		N_("Find usage of symbol in project"),
		G_CALLBACK (on_find_activate)
	},
	{
		"ActionPopupSymbolBrowserRefresh",
		GTK_STOCK_REFRESH,
		N_("_Refresh"),
		NULL,
		N_("Refresh symbol browser tree"),
		G_CALLBACK (on_refresh_activate)
	}
};

static void
on_project_element_added (IAnjutaProjectManager *pm, const gchar *uri,
						  SymbolBrowserPlugin *sv_plugin)
{
	gchar *filename;
	
	if (!sv_plugin->project_root_uri)
		return;
	
	filename = gnome_vfs_get_local_path_from_uri (uri);
	if (filename)
	{
		anjuta_symbol_view_add_source (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
									   filename);
		g_free (filename);
	}
}

static void
on_project_element_removed (IAnjutaProjectManager *pm, const gchar *uri,
							SymbolBrowserPlugin *sv_plugin)
{
	gchar *filename;
	
	if (!sv_plugin->project_root_uri)
		return;
	
	filename = gnome_vfs_get_local_path_from_uri (uri);
	if (filename)
	{
		anjuta_symbol_view_remove_source (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
										  filename);
		g_free (filename);
	}
}

// add a new project
static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaStatus *status;
	IAnjutaProjectManager *pm;
	SymbolBrowserPlugin *sv_plugin;
	const gchar *root_uri;

	sv_plugin = (SymbolBrowserPlugin *)plugin;
	  
	g_free (sv_plugin->project_root_uri);
	sv_plugin->project_root_uri = NULL;
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		if (root_dir)
		{
			status = anjuta_shell_get_status (plugin->shell, NULL);
			anjuta_status_progress_add_ticks (status, 1);
			trees_signals_block (sv_plugin);
			anjuta_symbol_view_open (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
									 root_dir);
			 
			/* Current editor symbol model may have changed */
			update_editor_symbol_model (sv_plugin);
			anjuta_status_progress_tick (status, NULL, _("Created symbols..."));
			trees_signals_unblock (sv_plugin);
			g_free (root_dir);
		}
		sv_plugin->project_root_uri = g_strdup (root_uri);
	}
	/* FIXME: There should be a way to ensure that this project manager
	 * is indeed the one that has opened the project_uri
	 */
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sv_plugin)->shell,
									 IAnjutaProjectManager, NULL);
	g_signal_connect (G_OBJECT (pm), "element_added",
					  G_CALLBACK (on_project_element_added), sv_plugin);
	g_signal_connect (G_OBJECT (pm), "element_removed",
					  G_CALLBACK (on_project_element_removed), sv_plugin);
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	IAnjutaProjectManager *pm;
	SymbolBrowserPlugin *sv_plugin;
	
	sv_plugin = (SymbolBrowserPlugin *)plugin;
	
	/* Disconnect events from project manager */
	
	/* FIXME: There should be a way to ensure that this project manager
	 * is indeed the one that has opened the project_uri
	 */
	pm = anjuta_shell_get_interface (ANJUTA_PLUGIN (sv_plugin)->shell,
									 IAnjutaProjectManager, NULL);
	g_signal_handlers_disconnect_by_func (G_OBJECT (pm),
										  on_project_element_added,
										  sv_plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (pm),
										  on_project_element_removed,
										  sv_plugin);
	
	/* clear anjuta_symbol_search side */
	anjuta_symbol_search_clear(ANJUTA_SYMBOL_SEARCH(sv_plugin->ss));

	/* clear glist's sfiles */
	anjuta_symbol_view_clear (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree));

	g_free (sv_plugin->project_root_uri);
	sv_plugin->project_root_uri = NULL;
}

static gboolean
on_treeview_event (GtkWidget *widget,
				   GdkEvent  *event,
				   SymbolBrowserPlugin *sv_plugin)
{
	GtkTreeView *view;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	g_return_val_if_fail (GTK_IS_TREE_VIEW (widget), FALSE);

	view = GTK_TREE_VIEW (widget);
	model = gtk_tree_view_get_model (view);
	selection = gtk_tree_view_get_selection (view); 

	if (!event)
		return FALSE;

	if (event->type == GDK_BUTTON_PRESS) {
		GdkEventButton *e = (GdkEventButton *) event;

		if (e->button == 3) {
			GtkWidget *menu;
			
			/* Popup project menu */
			menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (sv_plugin->ui),
											  "/PopupSymbolBrowser");
			gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
							NULL, NULL, e->button, e->time);
			return TRUE;
		}
	} else if (event->type == GDK_KEY_PRESS) {
		GdkEventKey *e = (GdkEventKey *) event;

		switch (e->keyval) {
			case GDK_Return:
				anjuta_ui_activate_action_by_group (sv_plugin->ui,
													sv_plugin->action_group,
											"ActionPopupSymbolBrowserGotoDef");
				return TRUE;
			default:
				return FALSE;
		}
	}
	return FALSE;
}

static void
on_treeview_row_activated (GtkTreeView *view, GtkTreePath *arg1,
						   GtkTreeViewColumn *arg2,
						   SymbolBrowserPlugin *sv_plugin)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection (view);
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	anjuta_ui_activate_action_by_group (sv_plugin->ui,
										sv_plugin->action_group,
										"ActionPopupSymbolBrowserGotoDef");
}

static void
trees_signals_block (SymbolBrowserPlugin *sv_plugin)
{
	g_signal_handlers_block_by_func (G_OBJECT (sv_plugin->sv_tree),
									 G_CALLBACK (on_treeview_event), NULL);

	g_signal_handlers_block_by_func (G_OBJECT (sv_plugin->ss),
									 G_CALLBACK (on_treesearch_symbol_selected_event),
									 NULL);
	
}

static void
trees_signals_unblock (SymbolBrowserPlugin *sv_plugin)
{
	g_signal_handlers_unblock_by_func (G_OBJECT (sv_plugin->sv_tree),
									 G_CALLBACK (on_treeview_event), NULL);
	
	g_signal_handlers_unblock_by_func (G_OBJECT (sv_plugin->ss),
									 G_CALLBACK (on_treesearch_symbol_selected_event),
									 NULL);
}

static void
on_symbol_selected (GtkAction *action, SymbolBrowserPlugin *sv_plugin)
{
	GtkTreeIter iter;
	
	if (egg_combo_action_get_active_iter (EGG_COMBO_ACTION (action), &iter))
	{
		gint line;
		line = anjuta_symbol_view_workspace_get_line (ANJUTA_SYMBOL_VIEW
													  (sv_plugin->sv_tree),
													  &iter);

		if (line > 0 && sv_plugin->current_editor)
		{
			/* Goto line number */
			ianjuta_editor_goto_line (IANJUTA_EDITOR (sv_plugin->current_editor),
									  line, NULL);
			if (IANJUTA_IS_MARKABLE (sv_plugin->current_editor))
			{
				ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (sv_plugin->current_editor),
													 IANJUTA_MARKABLE_BASIC,
													 NULL);

				ianjuta_markable_mark (IANJUTA_MARKABLE (sv_plugin->current_editor),
									   line, IANJUTA_MARKABLE_BASIC, NULL);
			}
		}
	}
}


/* -----------------------------------------------------------------------------
 * will manage the click of mouse and other events on search->hitlist treeview
 */
static void
on_treesearch_symbol_selected_event (AnjutaSymbolSearch *search,
									 AnjutaSymbolInfo *sym,
									 SymbolBrowserPlugin *sv_plugin) {
	gboolean ret;
	gint line;
	const gchar *file;
	
	ret = anjuta_symbol_view_get_file_symbol (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
											  sym->sym_name, TRUE,
											  &file, &line);
	if (ret)
	{
		goto_file_line (ANJUTA_PLUGIN (sv_plugin), file, line);
	}
}


static void
on_editor_destroy (SymbolBrowserPlugin *sv_plugin, IAnjutaEditor *editor)
{
	const gchar *uri;
	
	if (!sv_plugin->editor_connected || !sv_plugin->sv_tree)
		return;
	uri = g_hash_table_lookup (sv_plugin->editor_connected, G_OBJECT (editor));
	if (uri && strlen (uri) > 0)
	{
		DEBUG_PRINT ("Removing file tags of %s", uri);
		anjuta_symbol_view_workspace_remove_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
											   uri);
	}
	g_hash_table_remove (sv_plugin->editor_connected, G_OBJECT (editor));
}


static void
on_editor_saved (IAnjutaEditor *editor, const gchar *saved_uri,
				 SymbolBrowserPlugin *sv_plugin)
{
	const gchar *old_uri;
	gboolean tags_update;
	GtkTreeModel *file_symbol_model;
	GtkAction *action;
	AnjutaUI *ui;
	
	/* FIXME: Do this only if automatic tags update is enabled */
	/* tags_update =
		anjuta_preferences_get_int (te->preferences, AUTOMATIC_TAGS_UPDATE);
	*/
	tags_update = TRUE;
	if (tags_update)
	{
		gchar *local_filename;
		
		/* Verify that it's local file */
		local_filename = gnome_vfs_get_local_path_from_uri (saved_uri);
		g_return_if_fail (local_filename != NULL);
		g_free (local_filename);
		
		if (!sv_plugin->editor_connected)
			return;
	
		old_uri = g_hash_table_lookup (sv_plugin->editor_connected, editor);
		
		if (old_uri && strlen (old_uri) <= 0)
			old_uri = NULL;
		anjuta_symbol_view_workspace_update_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
												  old_uri, saved_uri);
		g_hash_table_insert (sv_plugin->editor_connected, editor,
							 g_strdup (saved_uri));
		
		/* Update File symbol view */
		ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (sv_plugin)->shell, NULL);
		action = anjuta_ui_get_action (ui, "ActionGroupSymbolNavigation",
									   "ActionGotoSymbol");
		
		
		file_symbol_model =
			anjuta_symbol_view_get_file_symbol_model (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree));
		g_object_set_data (G_OBJECT (editor), "tm_file",
						   g_object_get_data (G_OBJECT (file_symbol_model),
											  "tm_file"));
		
		egg_combo_action_set_model (EGG_COMBO_ACTION (action), file_symbol_model);
		if (gtk_tree_model_iter_n_children (file_symbol_model, NULL) > 0)
			g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
		else
			g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);

#if 0
		/* FIXME: Re-hilite all editors on tags update */
		if(update)
		{
			for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
			{
				te1 = (TextEditor *) tmp->data;
				text_editor_set_hilite_type(te1);
			}
		}
#endif
	}
}

static void
on_editor_foreach_disconnect (gpointer key, gpointer value, gpointer user_data)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT(key),
										  G_CALLBACK (on_editor_saved),
										  user_data);
	g_object_weak_unref (G_OBJECT(key),
						 (GWeakNotify) (on_editor_destroy),
						 user_data);
}

static void
on_editor_foreach_clear (gpointer key, gpointer value, gpointer user_data)
{
	const gchar *uri;
	SymbolBrowserPlugin *sv_plugin = (SymbolBrowserPlugin *)user_data;
	
	uri = (const gchar *)value;
	if (uri && strlen (uri) > 0)
	{
		DEBUG_PRINT ("Removing file tags of %s", uri);
		anjuta_symbol_view_workspace_remove_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree),
											   uri);
	}
}

static void
update_editor_symbol_model (SymbolBrowserPlugin *sv_plugin)
{
	AnjutaUI *ui;
	gchar *uri;
	GObject *editor = sv_plugin->current_editor;
	
	if (!editor)
		return;
	
	ui = anjuta_shell_get_ui (ANJUTA_PLUGIN (sv_plugin)->shell, NULL);
	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	if (uri)
	{
		gchar *local_filename;
		GtkTreeModel *file_symbol_model;
		GtkAction *action;
		
		/* Verify that it's local file */
		local_filename = gnome_vfs_get_local_path_from_uri (uri);
		g_return_if_fail (local_filename != NULL);
		g_free (local_filename);
		
		anjuta_symbol_view_workspace_add_file (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree), uri);
		action = anjuta_ui_get_action (ui, "ActionGroupSymbolNavigation",
									   "ActionGotoSymbol");
		
		file_symbol_model =
			anjuta_symbol_view_get_file_symbol_model (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree));
		if (file_symbol_model)
		{
			g_object_set_data (G_OBJECT (editor), "tm_file",
						   g_object_get_data (G_OBJECT (file_symbol_model),
											  "tm_file"));
			egg_combo_action_set_model (EGG_COMBO_ACTION (action), file_symbol_model);
		
		
			if (gtk_tree_model_iter_n_children (file_symbol_model, NULL) > 0)
				g_object_set (G_OBJECT (action), "sensitive", TRUE, NULL);
			else
				g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
		}
	}
	if (uri)
		g_free (uri);
}

static gboolean
on_editor_buffer_symbols_update_timeout (gpointer user_data)
{
	SymbolBrowserPlugin *sv_plugin;
	IAnjutaEditor *ed;
	gchar *current_buffer = NULL;
	gint buffer_size = 0;
	gchar *uri = NULL;

	sv_plugin = (SymbolBrowserPlugin*) user_data;
	
	if (sv_plugin->current_editor == NULL)
		return FALSE;
	 
	 /* we won't proceed with the updating of the symbols if we didn't type in 
	 	anything */
	 if (!need_symbols_update)
	 	return TRUE;
	
	if (sv_plugin->current_editor) {
		ed = IANJUTA_EDITOR (sv_plugin->current_editor);
		
		buffer_size = ianjuta_editor_get_length (ed, NULL);
		current_buffer = ianjuta_editor_get_text (ed, 0, -1, NULL);
				
		uri = ianjuta_file_get_uri (IANJUTA_FILE (ed), NULL);
		
	} 
	else
		return FALSE;
	
	if (uri) {
		anjuta_symbol_view_update_source_from_buffer (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree), 
				uri, current_buffer, buffer_size);
		g_free (uri);
	}
	
	if (current_buffer)
		g_free (current_buffer);  

	need_symbols_update = FALSE;

	return TRUE;
}

static void
on_char_added (IAnjutaEditor *editor, gint position, gchar ch,
				 SymbolBrowserPlugin *sv_plugin) {
	
	
	DEBUG_PRINT ("char added @ %d : %c [int %d]", position, ch, ch);
	
	/* try to force the update if a "." or a "->" is pressed */
	if ((ch == '.') || (prev_char_added == '-' && ch == '>'))
		on_editor_buffer_symbols_update_timeout (sv_plugin);
		
	need_symbols_update = TRUE;
	
	prev_char_added = ch;
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	gchar *uri;
	GObject *editor;
	SymbolBrowserPlugin *sv_plugin;
	
	editor = g_value_get_object (value);
	
	sv_plugin = (SymbolBrowserPlugin*)plugin;
	
	if (!sv_plugin->editor_connected)
	{
		sv_plugin->editor_connected = g_hash_table_new_full (g_direct_hash,
															 g_direct_equal,
															 NULL, g_free);
	}
	sv_plugin->current_editor = editor;
	
	update_editor_symbol_model (sv_plugin);
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	if (g_hash_table_lookup (sv_plugin->editor_connected, editor) == NULL)
	{
		g_object_weak_ref (G_OBJECT (editor),
						   (GWeakNotify) (on_editor_destroy),
						   sv_plugin);
		if (uri)
		{
			g_hash_table_insert (sv_plugin->editor_connected, editor,
								 g_strdup (uri));
		}
		else
		{
			g_hash_table_insert (sv_plugin->editor_connected, editor,
								 g_strdup (""));
		}
		g_signal_connect (G_OBJECT (editor), "saved",
						  G_CALLBACK (on_editor_saved),
						  sv_plugin);
						  
		g_signal_connect (G_OBJECT (editor), "char-added",
						  G_CALLBACK (on_char_added),
						  sv_plugin);
	}
	g_free (uri);
	
	/* add a default timeout to the updating of buffer symbols */	
	timeout_id = g_timeout_add (TIMEOUT_INTERVAL_SYMBOLS_UPDATE, on_editor_buffer_symbols_update_timeout, plugin);								 
	need_symbols_update = FALSE;
	
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	SymbolBrowserPlugin *sv_plugin;
	GtkAction *action;

	/* let's remove the timeout for symbols refresh */
	g_source_remove (timeout_id);
	need_symbols_update = FALSE;
	
	sv_plugin = (SymbolBrowserPlugin*)plugin;
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupSymbolNavigation",
								   "ActionGotoSymbol");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	sv_plugin->current_editor = NULL;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkActionGroup *group;
	GtkAction *action;
	SymbolBrowserPlugin *sv_plugin;
	
	DEBUG_PRINT ("SymbolBrowserPlugin: Activating Symbol Manager plugin...");
	
	register_stock_icons (plugin);
	
	sv_plugin = (SymbolBrowserPlugin*) plugin;
	sv_plugin->ui = anjuta_shell_get_ui (plugin->shell, NULL);
	sv_plugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);

	/* Create widgets */
	sv_plugin->sw = gtk_notebook_new();

	/* create symbol-view scrolled window */
	sv_plugin->sv = gtk_scrolled_window_new (NULL, NULL);
	sv_plugin->sv_tab_label = gtk_label_new (_("Tree" ));
	/* setting up some properties */
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sv_plugin->sv),
										 GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sv_plugin->sv),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	
	sv_plugin->sv_tree = anjuta_symbol_view_new ();
	g_object_add_weak_pointer (G_OBJECT (sv_plugin->sv_tree),
							   (gpointer*)&sv_plugin->sv_tree);

	g_signal_connect (G_OBJECT (sv_plugin->sv_tree), "event-after",
					  G_CALLBACK (on_treeview_event), plugin);
	g_signal_connect (G_OBJECT (sv_plugin->sv_tree), "row_activated",
					  G_CALLBACK (on_treeview_row_activated), plugin);

	gtk_container_add (GTK_CONTAINER(sv_plugin->sv), sv_plugin->sv_tree);
	/* add the scrolled window to the notebook */
	gtk_notebook_append_page (GTK_NOTEBOOK(sv_plugin->sw),
							  sv_plugin->sv, sv_plugin->sv_tab_label );

	/* anjuta symbol search */
	
	sv_plugin->ss =
		anjuta_symbol_search_new (ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree));
	sv_plugin->ss_tab_label = gtk_label_new (_("Search" ));

	g_object_add_weak_pointer (G_OBJECT (sv_plugin->ss),
							   (gpointer*)&sv_plugin->ss);

	gtk_notebook_append_page (GTK_NOTEBOOK(sv_plugin->sw), sv_plugin->ss,
							  sv_plugin->ss_tab_label );

	gtk_widget_show_all (sv_plugin->sw);

	/* connect some signals */
	g_signal_connect (G_OBJECT (sv_plugin->ss), "symbol_selected",
					  G_CALLBACK (on_treesearch_symbol_selected_event),
					  plugin);

	
	/* setting focus to the tree_view*/
	gtk_widget_grab_focus(sv_plugin->sv_tree);
	
	
	/* Add action group */
	sv_plugin->action_group = 
		anjuta_ui_add_action_group_entries (sv_plugin->ui,
											"ActionGroupSymbolBrowser",
											_("Symbol browser actions"),
											actions,
											G_N_ELEMENTS (actions),
											GETTEXT_PACKAGE, TRUE, plugin);
	sv_plugin->action_group = 
		anjuta_ui_add_action_group_entries (sv_plugin->ui,
											"ActionGroupPopupSymbolBrowser",
											_("Symbol browser popup actions"),
											popup_actions,
											G_N_ELEMENTS (popup_actions),
											GETTEXT_PACKAGE, FALSE, plugin);
	group = gtk_action_group_new ("ActionGroupSymbolNavigation");

	/* create a new combobox in style of libegg... */
	action = g_object_new (EGG_TYPE_COMBO_ACTION,
						   "name", "ActionGotoSymbol",
						   "label", _("Goto symbol"),
						   "tooltip", _("Select the symbol to go"),
						   "stock_id", GTK_STOCK_JUMP_TO,
							NULL);

	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	g_signal_connect (action, "activate",
					  G_CALLBACK (on_symbol_selected), sv_plugin);
	gtk_action_group_add_action (group, action);
	anjuta_ui_add_action_group (sv_plugin->ui, "ActionGroupSymbolNavigation",
								N_("Symbol navigations"), group, FALSE);
	sv_plugin->action_group_nav = group;
	
	/* Add UI */
	sv_plugin->merge_id = 
		anjuta_ui_merge (sv_plugin->ui, UI_FILE);
	
	/* Added widgets */
	anjuta_shell_add_widget (plugin->shell, sv_plugin->sw,
							 "AnjutaSymbolBrowser", _("Symbols"),
							 "symbol-browser-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	
	/* set up project directory watch */
	sv_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
									"project_root_uri",
									project_root_added,
									project_root_removed, NULL);
	sv_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	SymbolBrowserPlugin *sv_plugin;
	sv_plugin = (SymbolBrowserPlugin*) plugin;
	
	/* Ensure all editor cached info are released */
	if (sv_plugin->editor_connected)
	{
		g_hash_table_foreach (sv_plugin->editor_connected,
							  on_editor_foreach_disconnect, plugin);
		g_hash_table_foreach (sv_plugin->editor_connected,
							  on_editor_foreach_clear, plugin);
		g_hash_table_destroy (sv_plugin->editor_connected);
		sv_plugin->editor_connected = NULL;
	}
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, sv_plugin->root_watch_id, FALSE);
	anjuta_plugin_remove_watch (plugin, sv_plugin->editor_watch_id, TRUE);
	
	/* Remove widgets: Widgets will be destroyed when sw is removed */
	anjuta_shell_remove_widget (plugin->shell, sv_plugin->sw, NULL);
	
	/* Remove UI */
	anjuta_ui_unmerge (sv_plugin->ui, sv_plugin->merge_id);
	
	/* Remove action group */
	anjuta_ui_remove_action_group (sv_plugin->ui, sv_plugin->action_group);
	anjuta_ui_remove_action_group (sv_plugin->ui, sv_plugin->popup_action_group);
	anjuta_ui_remove_action_group (sv_plugin->ui, sv_plugin->action_group_nav);
	
	sv_plugin->root_watch_id = 0;
	sv_plugin->editor_watch_id = 0;
	sv_plugin->merge_id = 0;
	sv_plugin->sw = NULL;
	sv_plugin->sv = NULL;
	sv_plugin->sv_tree = NULL;
	sv_plugin->ss = NULL;
	
	return TRUE;
}

static void
dispose (GObject *obj)
{
	SymbolBrowserPlugin *sv_plugin = (SymbolBrowserPlugin*) obj;
	/* Ensure all editors are disconnected */
	if (sv_plugin->editor_connected)
	{
		g_hash_table_foreach (sv_plugin->editor_connected,
							  on_editor_foreach_disconnect,
							  sv_plugin);
		g_hash_table_destroy (sv_plugin->editor_connected);
		sv_plugin->editor_connected = NULL;
	}
	/*
	if (plugin->sw)
	{
		g_object_unref (G_OBJECT (plugin->sw));
		plugin->sw = NULL;
	}
	*/
	
	g_object_remove_weak_pointer (G_OBJECT (sv_plugin->ss),
										   (gpointer*)&sv_plugin->ss);

	
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
finalize (GObject *obj)
{
	/* SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin *) obj; */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
symbol_browser_plugin_instance_init (GObject *obj)
{
	SymbolBrowserPlugin *plugin = (SymbolBrowserPlugin*) obj;
	plugin->current_editor = NULL;
	plugin->editor_connected = NULL;
	plugin->sw = NULL;
	plugin->sv = NULL;
	plugin->gconf_notify_ids = NULL;
}

static void
symbol_browser_plugin_class_init (SymbolBrowserPluginClass *klass)
{
	GObjectClass *object_class;
	AnjutaPluginClass *plugin_class;
	
	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	plugin_class = (AnjutaPluginClass*) klass;
	
	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	
	object_class->dispose = dispose;
	object_class->finalize = finalize;
}

static IAnjutaIterable*
isymbol_manager_search (IAnjutaSymbolManager *sm,
						IAnjutaSymbolType match_types,
						const gchar *match_name,
						gboolean partial_name_match,
						gboolean global_search,
						GError **err)
{
	const GPtrArray *tags_array;
	AnjutaSymbolIter *iter = NULL;
	const gchar *name;
	
	if (match_name && strlen (match_name) > 0)
		name = match_name;
	else
		name = NULL;
	
	tags_array = tm_workspace_find (name, match_types, NULL,
									partial_name_match, global_search);
	if (tags_array)
	{
		iter = anjuta_symbol_iter_new (tags_array);
		return IANJUTA_ITERABLE (iter);
	}
	return NULL;
}

static IAnjutaIterable*
isymbol_manager_get_members (IAnjutaSymbolManager *sm,
							 const gchar *symbol_name,
							 gboolean global_search,
							 GError **err)
{
	const GPtrArray *tags_array;
	AnjutaSymbolIter *iter = NULL;
	
	tags_array = tm_workspace_find_scope_members (NULL, symbol_name,
												  global_search);
												  
	
	if (tags_array)
	{
		iter = anjuta_symbol_iter_new (tags_array);
		return IANJUTA_ITERABLE (iter);
	}
	return NULL;
}

static IAnjutaIterable*
isymbol_manager_get_parents (IAnjutaSymbolManager *sm,
							 const gchar *symbol_name,
							 GError **err)
{
	const GPtrArray *tags_array;
	AnjutaSymbolIter *iter = NULL;
	
	tags_array = tm_workspace_get_parents (symbol_name);
	if (tags_array)
	{
		iter = anjuta_symbol_iter_new (tags_array);
		return IANJUTA_ITERABLE (iter);
	}
	return NULL;
}

static IAnjutaIterable*
isymbol_manager_get_completions_at_position (IAnjutaSymbolManager *sm,
											const gchar *file_uri,
							 				const gchar *text_buffer, 
											gint text_length, 
											gint text_pos,
							 				GError **err)
{
	SymbolBrowserPlugin *sv_plugin;
	const TMTag *func_scope_tag;
	TMSourceFile *tm_file;
	IAnjutaEditor *ed;
	AnjutaSymbolView *symbol_view;
	gulong line;
	gulong scope_position;
	gchar *needed_text = NULL;
	gint access_method;
	GPtrArray * completable_tags_array;
	AnjutaSymbolIter *iter = NULL;
	
	sv_plugin = (SymbolBrowserPlugin*)sm;
	ed = IANJUTA_EDITOR (sv_plugin->current_editor);	
	symbol_view = ANJUTA_SYMBOL_VIEW (sv_plugin->sv_tree);
	
	line = ianjuta_editor_get_line_from_position (ed, text_pos, NULL);
	
	/* get the function scope */
	tm_file = anjuta_symbol_view_get_tm_file (symbol_view, file_uri);
	
	/* check whether the current file_uri is listed in the tm_workspace or not... */	
	if (tm_file == NULL)
		return 	NULL;
		

	// FIXME: remove DEBUG_PRINT	
/*/
	DEBUG_PRINT ("tags in file &s\n");
	if (tm_file->work_object.tags_array != NULL) {
		int i;
		for (i=0; i < tm_file->work_object.tags_array->len; i++) {
			TMTag *cur_tag;
		
			cur_tag = (TMTag*)g_ptr_array_index (tm_file->work_object.tags_array, i);
			tm_tag_print (cur_tag, stdout);
		}
	}
/*/

	func_scope_tag = tm_get_current_function (tm_file->work_object.tags_array, line);
		
	if (func_scope_tag == NULL) {
		DEBUG_PRINT ("func_scope_tag is NULL, seems like it's a completion on a global scope");
		return NULL;
	}
	
	DEBUG_PRINT ("current expression scope: %s", func_scope_tag->name);
	
	
	scope_position = ianjuta_editor_get_line_begin_position (ed, func_scope_tag->atts.entry.line, NULL);
	needed_text = ianjuta_editor_get_text (ed, scope_position,
										   text_pos - scope_position, NULL);

	if (needed_text == NULL)
		DEBUG_PRINT ("needed_text is null");
	DEBUG_PRINT ("text needed is %s", needed_text );
	

	/* we'll pass only the text of the current scope: i.e. only the current function
	 * in which we request the completion. */
	TMTag * found_type = anjuta_symbol_view_get_type_of_expression (symbol_view, 
				needed_text, text_pos - scope_position, func_scope_tag, &access_method);

				
	if (found_type == NULL) {
		DEBUG_PRINT ("type not found.");
		return NULL;	
	}
	
	/* get the completable memebers. If the access is COMPLETION_ACCESS_STATIC we don't
	 * want to know the parents members of the class.
	 */
	if (access_method == COMPLETION_ACCESS_STATIC)
		completable_tags_array = anjuta_symbol_view_get_completable_members (found_type, FALSE);
	else
		completable_tags_array = anjuta_symbol_view_get_completable_members (found_type, TRUE);
	
	if (completable_tags_array)
	{
		iter = anjuta_symbol_iter_new (completable_tags_array);
		return IANJUTA_ITERABLE (iter);
	}
	return NULL;
}


static void
isymbol_manager_iface_init (IAnjutaSymbolManagerIface *iface)
{
	iface->search = isymbol_manager_search;
	iface->get_members = isymbol_manager_get_members;
	iface->get_parents = isymbol_manager_get_parents;
	iface->get_completions_at_position = isymbol_manager_get_completions_at_position;
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	symbol_browser_prefs_init((SymbolBrowserPlugin*)ipref);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	symbol_browser_prefs_finalize ((SymbolBrowserPlugin*)ipref);
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (SymbolBrowserPlugin, symbol_browser_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (isymbol_manager, IANJUTA_TYPE_SYMBOL_MANAGER);
ANJUTA_PLUGIN_ADD_INTERFACE (ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (SymbolBrowserPlugin, symbol_browser_plugin);
