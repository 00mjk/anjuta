/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2000 Naba Kumar, 2005 Johannes Schmid

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
#include <gtk/gtkactiongroup.h>
#include <libgnome/gnome-i18n.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>

#include "plugin.h"
#include "project-import.h"

#define ICON_FILE "anjuta-project-import-plugin.png"

static gpointer parent_class;

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaProjectImportPlugin *iplugin;
	
#ifdef DEBUG
	g_message ("AnjutaProjectImportPlugin: Activating Project Import Plugin ...");
#endif
	
	iplugin = (AnjutaProjectImportPlugin*) plugin;
	iplugin->prefs = anjuta_shell_get_preferences (plugin->shell, NULL);
	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	AnjutaProjectImportPlugin *iplugin;
	iplugin = (AnjutaProjectImportPlugin*) plugin;
	return TRUE;
}

static void
dispose (GObject *obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
project_import_plugin_instance_init (GObject *obj)
{
	// AnjutaFileWizardPlugin *plugin = (AnjutaFileWizardPlugin*) obj;
}

static void
project_import_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}

static void
iwizard_activate (IAnjutaWizard *wiz, GError **err)
{
	AnjutaProjectImportPlugin* plugin = (AnjutaProjectImportPlugin*) wiz;
	ProjectImport* pi;
	
	pi = project_import_new(ANJUTA_PLUGIN(plugin));
}

static void
iwizard_iface_init (IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (AnjutaProjectImportPlugin, project_import_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (AnjutaProjectImportPlugin, project_import_plugin);
