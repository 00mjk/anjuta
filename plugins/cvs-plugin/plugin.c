/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar, Johannes Schmid

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-preferences.h>
#include <libanjuta/interfaces/ianjuta-vcs.h>

#include "plugin.h"
#include "cvs-actions.h"
#include "cvs-interface.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-cvs.ui"
#define ICON_FILE "anjuta-cvs-plugin-48.png"

static gpointer parent_class;

static GtkActionEntry actions_cvs[] = {
	{
		"ActionMenuCVS",          /* Action name */
		NULL,                     /* Stock icon, if any */
		N_("_CVS"),               /* Display label */
		NULL,                     /* short-cut */
		NULL,                     /* Tooltip */
		NULL
	},
	{
		"ActionCVSAdd",           /* Action name */
		GTK_STOCK_ADD,            /* Stock icon, if any */
		N_("_Add"),               /* Display label */
		NULL,                     /* short-cut */
		N_("Add a new file/directory to the CVS tree"), /* Tooltip */
		G_CALLBACK (on_menu_cvs_add) /* action callback */
	},
	{
		"ActionCVSRemove",        /* Action name */
		GTK_STOCK_REMOVE,         /* Stock icon, if any */
		N_("_Remove"),            /* Display label */
		NULL,                     /* short-cut */
		N_("Remove a file/directory from CVS tree"), /* Tooltip */
		G_CALLBACK (on_menu_cvs_remove) /* action callback */
	},
	{
		"ActionCVSCommit",        /* Action name */
		GTK_STOCK_YES,            /* Stock icon, if any */
		N_("_Commit"),            /* Display label */
		NULL,                     /* short-cut */
		N_("Commit your changes to the CVS tree"), /* Tooltip */
		G_CALLBACK (on_menu_cvs_commit) /* action callback */
	},
	{
		"ActionCVSUpdate",         /* Action name */
		GTK_STOCK_REFRESH,         /* Stock icon, if any */
		N_("_Update"),             /* Display label */
		"<control>u",              /* short-cut */
		N_("Sync your local copy with the CVS tree"), /* Tooltip */
		G_CALLBACK (on_menu_cvs_update) /* action callback */
	},
	{
		"ActionCVSDiff",           /* Action name */
		GTK_STOCK_ZOOM_100,        /* Stock icon, if any */
		N_("_Diff"),               /* Display label */
		NULL,                      /* short-cut */
		N_("Show differences between your local copy and the tree"),/*Tooltip*/
		G_CALLBACK (on_menu_cvs_diff) /* action callback */
	},
	{
		"ActionCVSStatus",         /* Action name */
		GTK_STOCK_ZOOM_100,        /* Stock icon, if any */
		N_("_Show Status"),        /* Display label */
		NULL,                      /* short-cut */
		N_("Show the status of a file/directory"), /* Tooltip */
		G_CALLBACK (on_menu_cvs_status) /* action callback */
	},
	{
		"ActionCVSLog",            /* Action name */
		GTK_STOCK_ZOOM_100,        /* Stock icon, if any */
		N_("_Show Log"),           /* Display label */
		NULL,                      /* short-cut */
		N_("Show the log of a file/directory"), /* Tooltip */
		G_CALLBACK (on_menu_cvs_log) /* action callback */
	},
	{
		"ActionCVSImport",         /* Action name */
		GTK_STOCK_ADD,             /* Stock icon, if any */
		N_("_Import Tree"),        /* Display label */
		NULL,                      /* short-cut */
		N_("Import a new source tree to CVS"), /* Tooltip */
		G_CALLBACK (on_menu_cvs_import) /* action callback */
	}
};

static GtkActionEntry popup_actions_cvs[] = {
	{
		"ActionPopupCVS",          /* Action name */
		NULL,                      /* Stock icon, if any */
		N_("_CVS"),                /* Display label */
		NULL,                      /* short-cut */
		NULL,                      /* Tooltip */
		NULL
	},
		{
		"ActionPopupCVSCommit",    /* Action name */
		GTK_STOCK_YES,             /* Stock icon, if any */
		N_("_Commit"),             /* Display label */
		NULL,                      /* short-cut */
		N_("Commit your changes to the CVS tree"), /* Tooltip */
		G_CALLBACK (on_fm_cvs_commit) /* action callback */
	},
	{
		"ActionPopupCVSUpdate",    /* Action name */
		GTK_STOCK_REFRESH,         /* Stock icon, if any */
		N_("_Update"),             /* Display label */
		NULL,                      /* short-cut */
		N_("Sync your local copy with the CVS tree"), /* Tooltip */
		G_CALLBACK (on_fm_cvs_update) /* action callback */
	},
	{
		"ActionPopupCVSDiff",      /* Action name */
		GTK_STOCK_ZOOM_100,        /* Stock icon, if any */
		N_("_Diff"),               /* Display label */
		NULL,                      /* short-cut */
		N_("Show differences between your local copy and the tree"),/*Tooltip*/
		G_CALLBACK (on_fm_cvs_diff) /* action callback */
	},
	{
		"ActionPopupCVSStatus",    /* Action name */
		GTK_STOCK_ZOOM_100,        /* Stock icon, if any */
		N_("_Show Status"),        /* Display label */
		NULL,                      /* short-cut */
		N_("Show the status of a file/directory"), /* Tooltip */
		G_CALLBACK (on_fm_cvs_status) /* action callback */
	},
	{
		"ActionPopupCVSLog",       /* Action name */
		GTK_STOCK_ZOOM_100,        /* Stock icon, if any */
		N_("_Show Log"),           /* Display label */
		NULL,                      /* short-cut */
		N_("Show the log of a file/directory"), /* Tooltip */
		G_CALLBACK (on_fm_cvs_log) /* action callback */
	}
};

static void
value_added_fm_current_uri (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *cvs_menu_action;
	const gchar *uri;
	GnomeVFSURI *cvs_uri = NULL;
	gchar *cvs_text_uri = NULL;
	gchar *cvs_dir;
	gchar *filename;
	GnomeVFSDirectoryHandle* handle;
	GnomeVFSFileInfo info;
	GnomeVFSResult result;
	
	uri = g_value_get_string (value);
	filename = gnome_vfs_get_local_path_from_uri (uri);
	g_return_if_fail (filename != NULL);

	CVSPlugin *cvs_plugin = ANJUTA_PLUGIN_CVS (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (cvs_plugin->fm_current_filename)
		g_free (cvs_plugin->fm_current_filename);
	cvs_plugin->fm_current_filename = filename;
	
	/* Show popup menu if CVS directory exists */
	cvs_menu_action = anjuta_ui_get_action (ui, "ActionGroupPopupCVS", "ActionPopupCVS");
	
	/* If a directory is selected we check if it contains a "CVS" directory,
	if it is a file we check if it's directory contains a "CVS" directory */
	result = gnome_vfs_get_file_info(uri, &info, 
		GNOME_VFS_FILE_INFO_DEFAULT);
	if (result == GNOME_VFS_OK)
	{
		if (info.type == GNOME_VFS_FILE_TYPE_DIRECTORY)
		{
			cvs_dir = g_strconcat(uri, "/CVS", NULL);
		}
		
		else
		{
			cvs_uri = gnome_vfs_uri_new (uri);
			cvs_text_uri = gnome_vfs_uri_extract_dirname(cvs_uri);
			cvs_dir = g_strconcat(cvs_text_uri, "/CVS", NULL);
			g_free(cvs_text_uri);
			gnome_vfs_uri_unref(cvs_uri);
		}	
	}
	else
		return; /* Strange... */
	if (gnome_vfs_directory_open(&handle, cvs_dir, 
		GNOME_VFS_FILE_INFO_DEFAULT) == GNOME_VFS_OK) 
	{
		
		g_object_set (G_OBJECT (cvs_menu_action), "sensitive", TRUE, NULL);
	}
	else
	{
		g_object_set (G_OBJECT (cvs_menu_action), "sensitive", FALSE, NULL);
	}
	g_free (cvs_dir);
}

static void
value_removed_fm_current_uri (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	AnjutaUI *ui;
	GtkAction *action;
	
	CVSPlugin *cvs_plugin = ANJUTA_PLUGIN_CVS (plugin);
	
	if (cvs_plugin->fm_current_filename)
		g_free (cvs_plugin->fm_current_filename);
	cvs_plugin->fm_current_filename = NULL;

	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	action = anjuta_ui_get_action (ui, "ActionGroupPopupCVS", "ActionPopupCVS");
	g_object_set (G_OBJECT (action), "sensitive", FALSE, NULL);
	
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	CVSPlugin *bb_plugin;
	const gchar *root_uri;

	bb_plugin = ANJUTA_PLUGIN_CVS (plugin);
	
	DEBUG_PRINT ("Project added");
	
	if (bb_plugin->project_root_dir)
		g_free (bb_plugin->project_root_dir);
	bb_plugin->project_root_dir = NULL;
	
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		bb_plugin->project_root_dir =
			gnome_vfs_get_local_path_from_uri (root_uri);
		if (bb_plugin->project_root_dir)
		{
			// update_project_ui (bb_plugin);
		}
	}
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer user_data)
{
	CVSPlugin *bb_plugin;

	bb_plugin = ANJUTA_PLUGIN_CVS (plugin);
	if (bb_plugin->project_root_dir)
		g_free (bb_plugin->project_root_dir);
	bb_plugin->project_root_dir = NULL;
	// update_project_ui (bb_plugin);
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	AnjutaUI *ui;
	gchar *uri;
	GObject *editor;
	
	editor = g_value_get_object (value);
	
	if (!IANJUTA_IS_EDITOR(editor))
		return;
	
	CVSPlugin *cvs_plugin = ANJUTA_PLUGIN_CVS (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	if (cvs_plugin->current_editor_filename)
		g_free (cvs_plugin->current_editor_filename);
	cvs_plugin->current_editor_filename = NULL;
	
	uri = ianjuta_file_get_uri (IANJUTA_FILE (editor), NULL);
	if (uri)
	{
		gchar *filename;
		
		filename = gnome_vfs_get_local_path_from_uri (uri);
		g_return_if_fail (filename != NULL);
		cvs_plugin->current_editor_filename = filename;
		g_free (uri);
		// update_module_ui (cvs_plugin);
	}
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	CVSPlugin *cvs_plugin = ANJUTA_PLUGIN_CVS (plugin);
	
	if (cvs_plugin->current_editor_filename)
		g_free (cvs_plugin->current_editor_filename);
	cvs_plugin->current_editor_filename = NULL;
	
	// update_module_ui (cvs_plugin);
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	CVSPlugin *cvs_plugin;
	
	DEBUG_PRINT ("CVSPlugin: Activating CVS plugin ...");
	cvs_plugin = ANJUTA_PLUGIN_CVS (plugin);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Add all our actions */
	cvs_plugin->cvs_action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupCVS",
											_("CVS operations"),
											actions_cvs,
											G_N_ELEMENTS (actions_cvs),
											GETTEXT_PACKAGE, TRUE, plugin);
	cvs_plugin->cvs_popup_action_group = 
		anjuta_ui_add_action_group_entries (ui, "ActionGroupPopupCVS",
											_("CVS popup operations"),
											popup_actions_cvs,
											G_N_ELEMENTS (popup_actions_cvs),
											GETTEXT_PACKAGE, FALSE, plugin);
	/* UI merge */
	cvs_plugin->uiid = anjuta_ui_merge (ui, UI_FILE);
	
	/* Add watches */
	cvs_plugin->fm_watch_id = 
		anjuta_plugin_add_watch (plugin, "file_manager_current_uri",
								 value_added_fm_current_uri,
								 value_removed_fm_current_uri, NULL);
	cvs_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	cvs_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	CVSPlugin *cvs_plugin;
	
	DEBUG_PRINT ("CVSPlugin: Dectivating CVS plugin ...");
	
	cvs_plugin = ANJUTA_PLUGIN_CVS (plugin);
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, cvs_plugin->fm_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, cvs_plugin->project_watch_id, TRUE);
	anjuta_plugin_remove_watch (plugin, cvs_plugin->editor_watch_id, TRUE);
	
	/* Unmerge UI */
	anjuta_ui_unmerge (ui, cvs_plugin->uiid);
	
	/* Remove action groups */
	anjuta_ui_remove_action_group (ui, cvs_plugin->cvs_action_group);
	anjuta_ui_remove_action_group (ui, cvs_plugin->cvs_popup_action_group);
	
	return TRUE;
}

static void
finalize (GObject *obj)
{
	// CVSPlugin *plugin = ANJUTA_PLUGIN_CVS (obj);
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
dispose (GObject *obj)
{
	// CVSPlugin *plugin = ANJUTA_PLUGIN_CVS (obj);
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
cvs_plugin_instance_init (GObject *obj)
{
	CVSPlugin *plugin = ANJUTA_PLUGIN_CVS (obj);
	plugin->uiid = 0;
	plugin->executing_command = FALSE;
	plugin->mesg_view = NULL;
	plugin->launcher = NULL;
	plugin->diff_editor = NULL;
	plugin->fm_current_filename = NULL;
	plugin->project_root_dir = NULL;
	plugin->current_editor_filename = NULL;
}

static void
cvs_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
	klass->finalize = finalize;
}

/* Interface */

static void
ianjuta_cvs_add (IAnjutaVcs *obj, const gchar* filename, 
	GError **err)
{
	anjuta_cvs_add(ANJUTA_PLUGIN(obj), filename, FALSE, err);
}
	
static void
ianjuta_cvs_commit (IAnjutaVcs *obj, const gchar* filename, const gchar* log, 
						 gboolean recurse, GError **err)
{
	anjuta_cvs_commit (ANJUTA_PLUGIN(obj), filename, log, "", recurse, err);
}

static void
ianjuta_cvs_remove (IAnjutaVcs *obj, const gchar* filename, GError **err)
{
	anjuta_cvs_remove (ANJUTA_PLUGIN(obj), filename, err);
}


static void
ianjuta_cvs_update (IAnjutaVcs *obj, const gchar* filename, gboolean recurse, GError **err)
{
	anjuta_cvs_update(ANJUTA_PLUGIN(obj), filename, recurse, FALSE, TRUE, FALSE,"", err);}

static void
ianjuta_vcs_iface_init (IAnjutaVcsIface *iface)
{
	iface->add = ianjuta_cvs_add;
	iface->remove = ianjuta_cvs_remove;
	iface->update = ianjuta_cvs_update;
	iface->commit = ianjuta_cvs_commit;	
}

static void
ipreferences_merge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	/* Create the messages preferences page */
	GladeXML* gxml;
	gxml = glade_xml_new (GLADE_FILE, "cvs", NULL);
	anjuta_preferences_add_page (prefs, gxml, "cvs", _("CVS"), ICON_FILE);
	g_object_unref (gxml);
}

static void
ipreferences_unmerge(IAnjutaPreferences* ipref, AnjutaPreferences* prefs, GError** e)
{
	anjuta_preferences_remove_page(prefs, _("CVS"));
}

static void
ipreferences_iface_init(IAnjutaPreferencesIface* iface)
{
	iface->merge = ipreferences_merge;
	iface->unmerge = ipreferences_unmerge;	
}

ANJUTA_PLUGIN_BEGIN (CVSPlugin, cvs_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(ianjuta_vcs, IANJUTA_TYPE_VCS);
ANJUTA_PLUGIN_ADD_INTERFACE(ipreferences, IANJUTA_TYPE_PREFERENCES);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (CVSPlugin, cvs_plugin);
