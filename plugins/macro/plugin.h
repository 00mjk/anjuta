/* 
 * 	plugin.h (c) 2004 Johannes Schmid
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include <config.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>

//#include "macro-db.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-macro.glade"

typedef struct _MacroPlugin MacroPlugin;
typedef struct _MacroPluginClass MacroPluginClass;
#include "macro-db.h"
struct _MacroPlugin
{
	AnjutaPlugin parent;
	
	/* Merge id */
	gint uiid;

	/* Action group */
	GtkActionGroup *action_group;
	
	/* Watch IDs */
	gint editor_watch_id;

	/* Watched values */
	GObject *current_editor;

	GtkWidget *macro_dialog;
	MacroDB *macro_db;
};

struct _MacroPluginClass
{
	AnjutaPluginClass parent_class;
};

gboolean
macro_insert (MacroPlugin * plugin, const gchar *keyword);

#endif
