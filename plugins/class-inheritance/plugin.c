/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * plugin.c
 * Copyright (C) Massimo Cora' 2005 <maxcvs@email.it>
 * 
 * plugin.c is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2, or (at your option) any later version.
 * 
 * plugin.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with plugin.c.  See the file "COPYING".  If not,
 * write to:  The Free Software Foundation, Inc.,
 *            59 Temple Place - Suite 330,
 *            Boston,  MA  02111-1307, USA.
 */

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnome/gnome-i18n.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>
#include <libanjuta/anjuta-debug.h>

#include "plugin.h"
#include "class-inherit.h"

static gpointer parent_class;


static void
project_root_added (AnjutaPlugin *plugin, const gchar *name,
					const GValue *value, gpointer user_data)
{
	AnjutaClassInheritance *ci_plugin;
	const gchar *root_uri;

	ci_plugin = (AnjutaClassInheritance*) plugin;
	root_uri = g_value_get_string (value);
	
	if (root_uri)
	{
		gchar *root_dir = gnome_vfs_get_local_path_from_uri (root_uri);
		if (root_dir)
		{	
			ci_plugin->top_dir = g_strdup(root_dir);
		}
		else
			ci_plugin->top_dir = NULL;
		g_free (root_dir);
	}
	else
		ci_plugin->top_dir = NULL;
	
	/* let's update the graph */
	class_inheritance_update_graph (ci_plugin);
}

static void
project_root_removed (AnjutaPlugin *plugin, const gchar *name,
					  gpointer user_data)
{
	AnjutaClassInheritance *ci_plugin;
	ci_plugin = (AnjutaClassInheritance*) plugin;
	
	if (ci_plugin->top_dir)
		g_free(ci_plugin->top_dir);
	ci_plugin->top_dir = NULL;
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{

	AnjutaUI *ui;
	AnjutaClassInheritance *class_inheritance;

	GtkWidget *wid;
	static gboolean initialized = FALSE;
	
	g_message ("AnjutaClassInheritance: Activating AnjutaClassInheritance plugin ...");
	class_inheritance = (AnjutaClassInheritance*) plugin;
	
	class_inheritance_base_gui_init (class_inheritance);
	
	anjuta_shell_add_widget (plugin->shell, class_inheritance->widget,
							 "AnjutaClassInheritance", _("Inheritance Graph"), NULL,
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
	class_inheritance->top_dir = NULL;
	
	/* set up project directory watch */
	class_inheritance->root_watch_id = anjuta_plugin_add_watch (plugin,
									"project_root_uri",
									project_root_added,
									project_root_removed, NULL);

	initialized	= TRUE;	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	g_message ("AnjutaClassInheritance: Dectivating AnjutaClassInheritance plugin ...");

	/* Container holds the last ref to this widget so it will be destroyed as
	 * soon as removed. No need to separately destroy it. */
	/* In most cases, only toplevel widgets (windows) require explicit 
	 * destruction, because when you destroy a toplevel its children will 
	 * be destroyed as well. */
	anjuta_shell_remove_widget (plugin->shell, ((AnjutaClassInheritance*)plugin)->widget,
								NULL);

	
	
	/* Remove watches */
	anjuta_plugin_remove_watch (plugin, ((AnjutaClassInheritance*)plugin)->root_watch_id, TRUE);
	return TRUE;
}

static void
class_inheritance_finalize (GObject *obj)
{
	AnjutaClassInheritance *ci_plugin;
	ci_plugin = (AnjutaClassInheritance *) obj;
	
	if (ci_plugin->top_dir)
		g_free (ci_plugin->top_dir);
	
	/* Finalization codes here */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
class_inheritance_dispose (GObject *obj)
{
	/* Disposition codes */
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
class_inheritance_instance_init (GObject *obj)
{
	AnjutaClassInheritance *plugin = (AnjutaClassInheritance*)obj;

	plugin->uiid = 0;

	plugin->widget = NULL;
	plugin->graph = NULL;
}

static void
class_inheritance_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->finalize = class_inheritance_finalize;
	klass->dispose = class_inheritance_dispose;
}

ANJUTA_PLUGIN_BOILERPLATE (AnjutaClassInheritance, class_inheritance);
ANJUTA_SIMPLE_PLUGIN (AnjutaClassInheritance, class_inheritance);
