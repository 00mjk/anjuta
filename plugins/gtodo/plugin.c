/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
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

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/resources.h>
#include <libanjuta/interfaces/ianjuta-todo.h>

//#include <libgtodo/main.h>
#include "main.h"
#include "plugin.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-gtodo.ui"
#define ICON_FILE "anjuta-gtodo-plugin.png"

static gpointer parent_class;

static void
on_hide_completed_action_activate (GtkAction *action, GTodoPlugin *plugin)
{
	gboolean state;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	gtodo_set_hide_done (state);
}

static void
on_hide_due_date_action_activate (GtkAction *action, GTodoPlugin *plugin)
{
	gboolean state;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	gtodo_set_hide_due (state);
}

static void
on_hide_end_date_action_activate (GtkAction *action, GTodoPlugin *plugin)
{
	gboolean state;
	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	gtodo_set_hide_nodate (state);
}

static GtkActionEntry actions_todo_view[] = {
	{
		"ActionMenuViewTodo",
		NULL,
		N_("_Tasks"),
		NULL, NULL, NULL,
	},
};

static GtkToggleActionEntry actions_view[] = {
	{
		"ActionViewTodoHideCompleted",
		NULL,
		N_("Hide _Completed Items"),
		NULL,
		N_("Hide completed todo items"),
		G_CALLBACK (on_hide_completed_action_activate),
		FALSE
	},
	{
		"ActionViewTodoHideDueDate",
		NULL,
		N_("Hide items that are past _due date"),
		NULL,
		N_("Hide items that are past due date"),
		G_CALLBACK (on_hide_due_date_action_activate),
		FALSE
	},
	{
		"ActionViewTodoHideEndDate",
		NULL,
		N_("Hide items without an _end date"),
		NULL,
		N_("Hide items without an end date"),
		G_CALLBACK (on_hide_end_date_action_activate),
		FALSE
	}
};

static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	const gchar *root_uri;

	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *todo_file;
		
		todo_file = g_strconcat (root_uri, "/TODO.tasks", NULL);
		gtodo_client_load (cl, todo_file);
		g_free (todo_file);
		category_changed();
	}
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	const gchar * home;
	gchar *default_todo;
	
	home = g_get_home_dir ();
	default_todo = g_strconcat ("file://", home, "/.gtodo/todos", NULL);
	
	gtodo_client_load (cl, default_todo);
	category_changed();
	g_free (default_todo);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	GtkWidget *wid;
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	GTodoPlugin *gtodo_plugin;
	GdkPixbuf *pixbuf;
	
	g_message ("GTodoPlugin: Activating Task manager plugin ...");
	gtodo_plugin = (GTodoPlugin*) plugin;
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	gtodo_load_settings();
	
	wid = gui_create_todo_widget();
	gtk_widget_show_all (wid);
	gtodo_plugin->widget = wid;
	
	pixbuf = anjuta_res_get_pixbuf (ICON_FILE);
	gtodo_plugin->prefs = preferences_widget();
	anjuta_preferences_dialog_add_page (ANJUTA_PREFERENCES_DIALOG (prefs),
								 "Todo Manager", pixbuf, gtodo_plugin->prefs);

	/* Add all our editor actions */
	anjuta_ui_add_action_group_entries (ui, "ActionGroupTodoView",
										_("Tasks manager"),
										actions_todo_view,
										G_N_ELEMENTS (actions_todo_view),
										plugin);
	anjuta_ui_add_toggle_action_group_entries (ui, "ActionGroupTodoViewOps",
										_("Tasks manager"),
										actions_view,
										G_N_ELEMENTS (actions_view),
										plugin);
	gtodo_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	anjuta_shell_add_widget (plugin->shell, wid,
							 "AnjutaTodoPlugin", _("Tasks"),
							 "gtodo", /* Icon stock */
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
	/* set up project directory watch */
	gtodo_plugin->root_watch_id = anjuta_plugin_add_watch (plugin,
													"project_root_uri",
													project_root_added,
													project_root_removed, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui = anjuta_shell_get_ui (plugin->shell, NULL);
	g_message ("GTodoPlugin: Dectivating Tasks manager plugin ...");
	anjuta_shell_remove_widget (plugin->shell, ((GTodoPlugin*)plugin)->widget,
								NULL);
	anjuta_ui_unmerge (ui, ((GTodoPlugin*)plugin)->uiid);
	anjuta_plugin_remove_watch (plugin,
								((GTodoPlugin*)plugin)->root_watch_id, TRUE);
	return TRUE;
}

static void
dispose (GObject *obj)
{
	// SamplePlugin *plugin = (SamplePlugin*)obj;
}

static void
gtodo_plugin_instance_init (GObject *obj)
{
	GTodoPlugin *plugin = (GTodoPlugin*)obj;
	plugin->uiid = 0;
}

static void
gtodo_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static void
itodo_load (IAnjutaTodo *profile, gchar *filename, GError **err)
{
	g_return_if_fail (filename != NULL);
	gtodo_client_load (cl, filename);
}

static void
itodo_iface_init(IAnjutaTodoIface *iface)
{
	iface->load = itodo_load;
}

ANJUTA_PLUGIN_BEGIN (GTodoPlugin, gtodo_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (itodo, IANJUTA_TYPE_TODO);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GTodoPlugin, gtodo_plugin);
