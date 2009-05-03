/*
 * macro_edit.h (c) 2005 Johannes Schmid
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef MACRO_EDIT_H
#define MACRO_EDIT_H

#include "plugin.h"
#include "macro-db.h"

#define MACRO_EDIT_TYPE            (macro_edit_get_type ())
#define MACRO_EDIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MACRO_EDIT_TYPE, MacroEdit))
#define MACRO_EDIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MACRO_EDIT_TYPE, MacroEditClass))
#define IS_MACRO_EDIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MACRO_EDIT_TYPE))
#define IS_MACRO_EDIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MACRO_EDIT_TYPE))


typedef struct _MacroEdit MacroEdit;
typedef struct _MacroEditClass MacroEditClass;

enum
{
	MACRO_ADD,
	MACRO_EDIT
};

struct _MacroEdit
{
	GtkDialog dialog;

	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	GtkWidget *name_entry;
	GtkWidget *category_entry;
	GtkWidget *shortcut_entry;
	GtkWidget *text;

	MacroDB *macro_db;
	GtkBuilder *bxml;
	gint type;
	GtkTreeSelection *select;
};

struct _MacroEditClass
{
	GtkDialogClass klass;

};

GType macro_edit_get_type (void);
GtkWidget *macro_edit_new (gint type, MacroDB * db);

void macro_edit_fill (MacroEdit * edit, GtkTreeSelection * selection);
void macro_edit_set_macro (MacroEdit* edit, const gchar* macro);
#endif
