/*
    compiler_options.c
    Copyright (C) 2000  Kh. Naba Kumar Singh

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gnome.h>
#include "anjuta.h"
#include "resources.h"
#include "compiler_options.h"
#include "../pixmaps/list_unselect.xpm"
#include "../pixmaps/list_select.xpm"

#define ANJUTA_SUPPORTS_END_STRING "SUPPORTS_END"
#define ANJUTA_SUPPORTS_END \
  { \
    ANJUTA_SUPPORTS_END_STRING, NULL, NULL, \
    NULL, NULL, NULL, NULL, NULL, NULL, NULL \
  }

gchar *anjuta_supports[][ANJUTA_SUPPORT_END_MARK] = {
	{
	 "GLIB",
	 "Glib library for most C utilities",
	 "",
	 "AM_MISSING_PROG(GLIB_CONFIG, glib-config, $mising_dir)\n",
	 "`glib-config --cflags`",
	 "`glib-config --libs`",
	 "`glib-config --cflags`",
	 "`glib-config --libs`",
	 "",
	 "glib-config --version"}
	,
	{
	 "GTK",
	 "Gimp Toolkit library for GUI development",
	 "",
	 "",
	 "gtk-config --cflags",
	 "$(GTK_LIBS)",
	 "`gtk-config --cflags`",
	 "`gtk-config --libs`",
	 "",
	 "gtk-config --version"}
	,
	{
	 "GNOME",
	 "GNOME is Computing Made Easy",
	 "",
	 "",
	 "$(GNOME_INCLUDESDIR)",
	 "$(GNOME_LIBDIR) $(GNOME_LIBS)",
	 "`gnome-config --cflags glib gtk gnome gnomeui`",
	 "`gnome-config --libs glib gtk gnome gnomeui`",
	 "",
	 "gnome-config --version"}
	,
	{
	 "BONOBO",
	 "GNOME Bonobo component for fast integration",
	 "",
	 "",
	 "$(BONOBO_CFLAGS)",
	 "$(BONOBO_LIBS)",
	 "`gnome-config --cflags glib gtk gnome gnomeui bonobo`",
	 "`gnome-config --libs glib gtk gnome gnomeui bonobo`",
	 "",
	 "gnome-config --version"}
	,
	{
	"gtkmm",
	 "C++ Bindings for GTK",
	 "",
	 "",
	 "$(GTKMM_CFLAGS)",
	 "$(GTKMM_LIBS)",
	 "`gtkmm-config --cflags`",
	 "`gtkmm-config --libs`",
	 "",
	 "gtkmm-config --version"}
	 ,
	{
	 "gnomemm",
	 "C++ bindings for GNOME",
	 "",
	 "",
	 "$(GNOMEMM_CFLAGS)",
	 "$(GNOMEMM_LIBS)",
	 "`gnome-config --cflags gnomemm`",
	 "`gnome-config --libs gnomemm`",
	 "",
	 "gnome-config --version"}
	 ,
	{
	 "LIBGLADE",
	 "C program using libglade",
	 "",
	 "",
	 "$(LIBGLADE_CFLAGS)",
	 "$(LIBGLADE_LIBS)",
	 "`libglade-config --cflags gnome`",
	 "`libglade-config --libs gnome`",
	 "",
	 "libglade-config --version"}
	 ,
	{
	 "WXWINDOWS",
	 "C++ program using wxWindows (wxGTK) toolkit",
	 "",
	 "",
	 "$(WX_CXXFLAGS)",
	 "$(WX_LIBS)",
	 "`wx-config --cxxflags`",
	 "`wx-config --libs`",
	 "",
	 "wx-config --version"}
	,
	ANJUTA_SUPPORTS_END
};

static gchar *anjuta_warnings[] = {
	" -Werror",                    "Consider warnings as errors",
	" -w",                         "No warnings",
	" -Wall",                      "Enable most warnings",
	" -Wimplicit",                 "Warning for implicit declarations",
	" -Wreturn-type",              "Warning for mismatched return types",
	" -Wunused",                   "Warning for unused variables",
	" -Wswitch",                "Warning for unhandled switch case with enums",
	" -Wcomment",                  "Warning for nested comments",
	" -Wuninitialized",            "Warning for uninitialized variable use",
	" -Wparentheses",              "Warning for missing parentheses",
	" -Wtraditional",         "Warning for differences to traditionanl syntax",
	" -Wshadow",                   "Warning for variable shadowing",
	" -Wpointer-arith",            "Warning for missing prototypes",
	" -Wmissing-prototypes",       "Warning for suspected pointer arithmetic",
	" -Winline",                   "Warning if declarations cannot be inlined",
	" -Woverloaded-virtual",        "Warning for overloaded virtuals",
	NULL, NULL
};

static gchar *anjuta_defines[] = {
	"HAVE_CONFIG_H",
	"DEBUG_LEVEL_2",
	"DEBUG_LEVEL_1",
	"DEBUG_LEVEL_0",
	"RELEASE",
	NULL,
};

static gchar *optimize_button_option[] = {
	"",
	" -O1",
	" -O2",
	" -O3"
};

static gchar *other_button_option[] = {
	" -g",
	" -pg",
};

enum {
	SUPP_TOGGLE_COLUMN,	
	SUPP_DESCRIPTION_COLUMN,
	N_SUPP_COLUMNS
};

enum {
	INC_PATHS_COLUMN,
	N_INC_COLUMNS
};

enum {
	LIB_PATHS_COLUMN,
	N_LIB_PATHS_COLUMN
};

enum {
	LIB_TOGGLE_COLUMN,
	LIB_COLUMN,
	N_LIB_COLUMNS
};

enum {
	LIB_STOCK_COLUMN,
	LIB_STOCK_DES_COLUMN,
	N_LIB_STOCK_COLUMNS
};

enum {
	DEF_TOGGLE_COLUMN,
	DEF_DEFINE_COLUMN,
	N_DEF_COLUMNS
};

enum {
	DEF_STOCK_COLUMN,
	N_DEF_STOCK_COLUMNS
};

enum {
	WARNINGS_TOGGLE_COLUMN,
	WARNINGS_COLUMN,
	WARNINGS_DES_COLUMN,
	N_WARNINGS_COLUMNS
};

static void co_cid_set (GtkListStore* store, GtkTreeIter *iter, gboolean state)
{
	// cid->state = state;
}

static gboolean co_cid_get (GtkListStore* store, GtkTreeIter *iter)
{
	// return cid->state;
	return FALSE;
}

static gboolean co_cid_toggle (GtkListStore* store, GtkTreeIter *iter)
{
	return FALSE;
}

static void
populate_stock_libs (GtkListStore *tmodel)
{
	gint i, ch;
	gchar *stock_file;
	gchar lib_name[256];
	gchar desbuff[512];
	FILE *fp;
	
	/* Now read the stock libraries */
	stock_file =
		g_strconcat (app->dirs->data, "/stock_libs.anj",
				 NULL);
	fp = fopen (stock_file, "r");
	g_free (stock_file);

	if (fp == NULL)
		goto down;

	/* Skip the first line which is comment */
	do
	{
		ch = fgetc (fp);
		if (ch == EOF)
			goto down;
	}
	while (ch != '\n');

	while (feof (fp) == FALSE)
	{
		GtkTreeIter iter;
		
		/* Read a line at a time */
		fscanf (fp, "%s", lib_name);	/* The lib name */

		/* Followed by description */
		i = 0;
		do
		{
			ch = fgetc (fp);
			if (ch == EOF)
				goto down;	/* Done when eof */
			desbuff[i] = (gchar) ch;
			i++;
		}
		while (ch != '\n');

		desbuff[--i] = '\0';
		
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter,
							LIB_STOCK_COLUMN, lib_name,
							LIB_STOCK_DES_COLUMN, desbuff,
							-1);
	}

  down:
	if (fp)
		fclose (fp);
}

static void
populate_supports (GtkListStore *tmodel)
{
	int i = 0;

	/* Now setup all the available supports */
	while (strcmp (anjuta_supports[i][ANJUTA_SUPPORT_ID],
				   ANJUTA_SUPPORTS_END_STRING) != 0)
	{
		GtkTreeIter iter;
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter, 
							SUPP_TOGGLE_COLUMN, FALSE,
							SUPP_DESCRIPTION_COLUMN, anjuta_supports[i][1],
							-1);
		i++;
	}
}

static void
populate_stock_defs (GtkListStore *tmodel)
{
	/* Now setup buildin defines */
	int i = 0;
	while (anjuta_defines[i])
	{
		GtkTreeIter iter;
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter,
							DEF_STOCK_COLUMN, anjuta_defines[i],
							-1);
		i++;
	}
}

static void
populate_warnings (GtkListStore *tmodel)
{
	/* Now setup buildin defines */
	int i = 0;
	while (anjuta_warnings[i])
	{
		GtkTreeIter iter;
		gtk_list_store_append (tmodel, &iter);
		gtk_list_store_set (tmodel, &iter,
							WARNINGS_TOGGLE_COLUMN, FALSE,
							WARNINGS_COLUMN, anjuta_warnings[i],
							WARNINGS_DES_COLUMN, anjuta_warnings[i+1],
							-1);
		i += 2;
	}
}

void create_compiler_options_gui (CompilerOptions *co)
{
	GtkTreeView *clist;
	GtkListStore *store, *list;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	int i;
	
	/* Compiler Options Dialog */
	co->gxml = glade_xml_new (GLADE_FILE_ANJUTA, "compiler_options_dialog", NULL);
	glade_xml_signal_autoconnect (co->gxml);
	co->widgets.window = glade_xml_get_widget (co->gxml, "compiler_options_dialog");
	gtk_widget_hide (co->widgets.window);
	co->widgets.supp_clist = glade_xml_get_widget (co->gxml, "supp_clist");
	co->widgets.inc_clist = glade_xml_get_widget (co->gxml, "inc_clist");
	co->widgets.inc_entry = glade_xml_get_widget (co->gxml, "inc_entry");
	co->widgets.lib_paths_clist = glade_xml_get_widget (co->gxml, "lib_paths_clist");
	co->widgets.lib_paths_entry = glade_xml_get_widget (co->gxml, "lib_paths_entry");
	co->widgets.lib_clist = glade_xml_get_widget (co->gxml, "lib_clist");
	co->widgets.lib_stock_clist = glade_xml_get_widget (co->gxml, "lib_stock_clist");
	co->widgets.lib_entry = glade_xml_get_widget (co->gxml, "lib_entry");
	co->widgets.def_clist = glade_xml_get_widget (co->gxml, "def_clist");
	co->widgets.def_stock_clist = glade_xml_get_widget (co->gxml, "def_stock_clist");
	co->widgets.def_entry = glade_xml_get_widget (co->gxml, "def_entry");
	co->widgets.warnings_clist = glade_xml_get_widget (co->gxml, "warnings_clist");
	co->widgets.optimize_button[0] = glade_xml_get_widget (co->gxml, "optimization_b_1");
	co->widgets.optimize_button[1] = glade_xml_get_widget (co->gxml, "optimization_b_2");
	co->widgets.optimize_button[2] = glade_xml_get_widget (co->gxml, "optimization_b_3");
	co->widgets.optimize_button[3] = glade_xml_get_widget (co->gxml, "optimization_b_4");
	co->widgets.other_button[0] = glade_xml_get_widget (co->gxml, "debugging_b");
	co->widgets.other_button[1] = glade_xml_get_widget (co->gxml, "profiling_b");
	co->widgets.other_c_flags_entry = glade_xml_get_widget (co->gxml, "other_c_flags_entry");
	co->widgets.other_l_flags_entry = glade_xml_get_widget (co->gxml, "other_l_flags_entry");
	co->widgets.other_l_libs_entry = glade_xml_get_widget (co->gxml, "other_l_libs_entry");

	/* Add supports tree model */
	clist = GTK_TREE_VIEW (co->widgets.supp_clist);
	store = gtk_list_store_new (N_SUPP_COLUMNS, G_TYPE_BOOLEAN,
								G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add supports columns */	
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
													   "boolean",
													   SUPP_TOGGLE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
													   renderer,
													   "text",
													   SUPP_DESCRIPTION_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	populate_supports (store);
	g_object_unref (G_OBJECT(store));
	
	/* Add "includes" tree model */
	clist = GTK_TREE_VIEW (co->widgets.inc_clist);
	store = gtk_list_store_new (N_INC_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model (clist, GTK_TREE_MODEL(store));
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Include paths"),
													   renderer,
													   "text",
													   INC_PATHS_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	g_object_unref (G_OBJECT(store));
	
	/* Add "libraries paths" model */
	clist = GTK_TREE_VIEW (co->widgets.lib_paths_clist);
	store = gtk_list_store_new (N_LIB_PATHS_COLUMN, G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add "libraries paths" columns */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Libraries paths"),
												   renderer,
												   "text",
												   LIB_PATHS_COLUMN,
												   NULL);
	gtk_tree_view_append_column (clist, column);
	g_object_unref (G_OBJECT(store));
	
	/* Add "includes" tree model */
	clist = GTK_TREE_VIEW (co->widgets.lib_clist);
	store = gtk_list_store_new (N_LIB_COLUMNS, G_TYPE_BOOLEAN,
								G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add "libraries paths" columns */
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
													   "boolean",
													   LIB_TOGGLE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column =
	gtk_tree_view_column_new_with_attributes (_("Libraries and modules"),
												   renderer,
												   "text",
												   LIB_COLUMN,
												   NULL);
	gtk_tree_view_append_column (clist, column);
	g_object_unref (G_OBJECT(store));
		
	/* Add "Libraries stock" model */
	clist = GTK_TREE_VIEW (co->widgets.lib_stock_clist);
	store = gtk_list_store_new (N_LIB_STOCK_COLUMNS, G_TYPE_STRING,
								G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add "Libraries stock" column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Stock"),
												   renderer,
												   "text",
												   LIB_STOCK_COLUMN,
												   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
												   renderer,
												   "text",
												   LIB_STOCK_DES_COLUMN,
												   NULL);
	gtk_tree_view_append_column (clist, column);
	populate_stock_libs (store);
	g_object_unref (G_OBJECT(store));
	
	/* Add "Defines" model */
	clist = GTK_TREE_VIEW (co->widgets.def_clist);
	store = gtk_list_store_new (N_DEF_COLUMNS, G_TYPE_BOOLEAN,
								G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add defines columns */	
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
													   "boolean",
													   DEF_TOGGLE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
													   renderer,
													   "text",
													   DEF_DEFINE_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	g_object_unref (G_OBJECT(store));

	/* Add "Defines stock" model */
	clist = GTK_TREE_VIEW (co->widgets.def_stock_clist);
	store = gtk_list_store_new (N_DEF_STOCK_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model (clist,	GTK_TREE_MODEL(store));
	
	/* Add defines columns */	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Stock Defines"),
													   renderer,
													   "text",
													   DEF_STOCK_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	populate_stock_defs (store);
	g_object_unref (G_OBJECT(store));
	
	/* Add "warnings" model */
	clist = GTK_TREE_VIEW (co->widgets.warnings_clist);
	store = gtk_list_store_new (N_WARNINGS_COLUMNS, G_TYPE_BOOLEAN,
								G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (clist, GTK_TREE_MODEL(store));
	
	/* Add defines columns */	
	renderer = gtk_cell_renderer_toggle_new ();
	column = gtk_tree_view_column_new_with_attributes ("", renderer,
													"boolean",
													WARNINGS_TOGGLE_COLUMN,
													NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Warning"),
													   renderer,
													   "text",
													   WARNINGS_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Description"),
													   renderer,
													   "text",
													   WARNINGS_DES_COLUMN,
													   NULL);
	gtk_tree_view_append_column (clist, column);
	populate_warnings (store);
	g_object_unref (G_OBJECT(store));
	
	gtk_widget_ref (co->widgets.window);
	gtk_widget_ref (co->widgets.warnings_clist);
	for (i = 0; i < 4; i++)
		gtk_widget_ref (co->widgets.optimize_button[i]);
	for (i = 0; i < 2; i++)
		gtk_widget_ref (co->widgets.other_button[i]);
	
	gtk_widget_ref (co->widgets.inc_clist);
	gtk_widget_ref (co->widgets.inc_entry);
	gtk_widget_ref (co->widgets.lib_paths_clist);
	gtk_widget_ref (co->widgets.lib_paths_entry);
	gtk_widget_ref (co->widgets.lib_clist);
	gtk_widget_ref (co->widgets.lib_stock_clist);
	gtk_widget_ref (co->widgets.lib_entry);
	gtk_widget_ref (co->widgets.def_clist);
	gtk_widget_ref (co->widgets.def_stock_clist);
	gtk_widget_ref (co->widgets.def_entry);
	gtk_widget_ref (co->widgets.other_c_flags_entry);
	gtk_widget_ref (co->widgets.other_l_flags_entry);
	gtk_widget_ref (co->widgets.other_l_libs_entry);
	gtk_widget_ref (co->widgets.supp_clist);
}

CompilerOptions *
compiler_options_new (PropsID props)
{
	int i;
	CompilerOptions *co = g_new0 (CompilerOptions, 1);

	if (co)
	{
		co->supp_index = 0;
		co->inc_index = 0;
		co->lib_index = 0;
		co->lib_paths_index = 0;
		co->def_index = 0;
		co->is_showing = FALSE;
		co->win_pos_x = 100;
		co->win_pos_y = 100;
		co->props = props;
		create_compiler_options_gui (co);
	}
	return co;
}

void
compiler_options_destroy (CompilerOptions * co)
{
	gint i;
	if (co)
	{
		gtk_widget_unref (co->widgets.window);
		gtk_widget_unref (co->widgets.warnings_clist);
		for (i = 0; i < 4; i++)
			gtk_widget_unref (co->widgets.optimize_button[i]);
		for (i = 0; i < 2; i++)
			gtk_widget_unref (co->widgets.other_button[i]);

		gtk_widget_unref (co->widgets.inc_clist);
		gtk_widget_unref (co->widgets.inc_entry);
		gtk_widget_unref (co->widgets.lib_paths_clist);
		gtk_widget_unref (co->widgets.lib_paths_entry);
		gtk_widget_unref (co->widgets.lib_clist);
		gtk_widget_unref (co->widgets.lib_stock_clist);
		gtk_widget_unref (co->widgets.lib_entry);
		gtk_widget_unref (co->widgets.def_clist);
		gtk_widget_unref (co->widgets.def_stock_clist);
		gtk_widget_unref (co->widgets.def_entry);
		gtk_widget_unref (co->widgets.other_c_flags_entry);
		gtk_widget_unref (co->widgets.other_l_flags_entry);
		gtk_widget_unref (co->widgets.other_l_libs_entry);
		gtk_widget_unref (co->widgets.supp_clist);

		if (co->widgets.window)
			gtk_widget_destroy (co->widgets.window);
		g_object_unref (co->gxml);
		g_free (co);
		co = NULL;
	}
}

gboolean compiler_options_save_yourself (CompilerOptions * co, FILE * stream)
{
	if (stream == NULL || co == NULL)
		return FALSE;
	fprintf (stream, "compiler.options.win.pos.x=%d\n", co->win_pos_x);
	fprintf (stream, "compiler.options.win.pos.y=%d\n", co->win_pos_y);
	return TRUE;
}

gint compiler_options_save (CompilerOptions * co, FILE * s)
{
#if 0
	gchar *text;
	gint length, i;

	g_return_val_if_fail (co != NULL, FALSE);
	g_return_val_if_fail (s != NULL, FALSE);

	length = g_list_length (GTK_CLIST (co->widgets.supp_clist)->row_list);
	fprintf (s, "compiler.options.supports=");
	for (i = 0; i < length; i++)
	{
		gboolean state;
		state = co_clist_row_data_get_state (GTK_CLIST (co->widgets.supp_clist), i);
		if(state)
		{
			if (fprintf (s, "%s ", anjuta_supports[i][ANJUTA_SUPPORT_ID]) <1)
			{
				return FALSE;
			}
		}
	}
	fprintf (s, "\n");
	
	length = g_list_length (GTK_CLIST (co->widgets.inc_clist)->row_list);
	fprintf (s, "compiler.options.include.paths=");
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.inc_clist), i, 0, &text);
		if (fprintf (s, "\\\n\t%s", text) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	length =
		g_list_length (GTK_CLIST (co->widgets.lib_paths_clist)->
			       row_list);
	fprintf (s, "compiler.options.library.paths=");
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.lib_paths_clist),
				    i, 0, &text);
		if (fprintf (s, "\\\n\t%s", text) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	length = g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list);
	fprintf (s, "compiler.options.libraries=");
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.lib_clist), i, 1, &text);
		if (fprintf (s, "\\\n\t%s", text) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	length = g_list_length (GTK_CLIST (co->widgets.lib_clist)->row_list);
	fprintf (s, "compiler.options.libraries.selected=");
	for (i = 0; i < length; i++)
	{
		gboolean state;
		state = co_clist_row_data_get_state (GTK_CLIST (co->widgets.lib_clist), i);
		if (fprintf (s, "%d ", state) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	length = g_list_length (GTK_CLIST (co->widgets.def_clist)->row_list);
	fprintf (s, "compiler.options.defines=");
	for (i = 0; i < length; i++)
	{
		gtk_clist_get_text (GTK_CLIST (co->widgets.def_clist), i, 0,
				    &text);
		if (fprintf (s, "\\\n\t%s", text) <1)
		{
			return FALSE;
		}
	}
	fprintf (s, "\n");

	fprintf (s, "compiler.options.warning.buttons=");
	for (i = 0; i < 16; i++)
		fprintf (s, "%d ", (int) co->warning_button_state[i]);
	fprintf (s, "\n");

	fprintf (s, "compiler.options.optimize.buttons=");
	for (i = 0; i < 4; i++)
		fprintf (s, "%d ", (int) co->optimize_button_state[i]);
	fprintf (s, "\n");

	fprintf (s, "compiler.options.other.buttons=");
	for (i = 0; i < 2; i++)
		fprintf (s, "%d ", (int) co->other_button_state[i]);
	fprintf (s, "\n");

	fprintf (s, "compiler.options.other.c.flags=%s\n",
		 co->other_c_flags);
	
	fprintf (s, "compiler.options.other.l.flags=%s\n",
		 co->other_l_flags);
	
	fprintf (s, "compiler.options.other.l.libs=%s\n",
		 co->other_l_libs);

	return TRUE;
#endif
}

gboolean compiler_options_load_yourself (CompilerOptions * co, PropsID props)
{
	if (co == NULL)
		return FALSE;
	co->win_pos_x =
		prop_get_int (props, "compiler.options.win.pos.x", 100);
	co->win_pos_y =
		prop_get_int (props, "compiler.options.win.pos.y", 100);
	return TRUE;
}

#define CLIST_CLEAR_ALL(widget) \
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget)); \
	g_assert (model); \
	if (model) gtk_list_store_clear (GTK_LIST_STORE (model));

#define CLIST_APPEND_STRING_ALL(property, widget, col) \
	list = glist_from_data (props, property); \
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget)); \
	g_assert (model); \
	node = list; \
	while (node) \
	{ \
		gtk_list_store_append (GTK_LIST_STORE(model), &iter); \
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, col, node->data, -1); \
		node = g_list_next (node); \
	} \
	glist_strings_free (list);

#define CLIST_UPDATE_BOOLEAN_ALL(property, widget, col) \
	list = glist_from_data (props, property); \
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget)); \
	g_assert (model); \
	valid = gtk_tree_model_get_iter_first (model, &iter); \
	node = list; \
	while (node && valid) \
	{ \
		int value = atoi(node->data); \
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, col, value, -1); \
		node = g_list_next (node); \
		valid = gtk_tree_model_iter_next (model, &iter); \
	} \
	glist_strings_free (list);

void
compiler_options_clear(CompilerOptions *co)
{
	gint i;
	GtkTreeModel* model;
	GtkTreeIter iter;
	gboolean valid;

	g_return_if_fail (co != NULL);

	gtk_widget_set_sensitive (co->widgets.supp_clist, TRUE);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->widgets.supp_clist));
	g_assert (model);
	
	/* Clear all supports */
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, FALSE, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	CLIST_CLEAR_ALL(co->widgets.inc_clist);
	CLIST_CLEAR_ALL(co->widgets.lib_paths_clist);
	CLIST_CLEAR_ALL(co->widgets.lib_clist);
	CLIST_CLEAR_ALL(co->widgets.def_clist);
	
	gtk_entry_set_text (GTK_ENTRY (co->widgets.inc_entry), "");
	gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_paths_entry), "");
	gtk_entry_set_text (GTK_ENTRY (co->widgets.lib_entry), "");
	gtk_entry_set_text (GTK_ENTRY (co->widgets.def_entry), "");
	
	for (i = 0; i < 4; i++)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(co->widgets.optimize_button[i]), FALSE);
	for (i = 0; i < 2; i++)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(co->widgets.other_button[i]), FALSE);

	co->supp_index = 0;
	co->inc_index = 0;
	co->lib_index = 0;
	co->lib_paths_index = 0;
	co->def_index = 0;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->widgets.warnings_clist));
	g_assert (model);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, FALSE, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

	gtk_entry_set_text (GTK_ENTRY(co->widgets.other_c_flags_entry), "");
	gtk_entry_set_text (GTK_ENTRY(co->widgets.other_l_flags_entry), "");
	gtk_entry_set_text (GTK_ENTRY(co->widgets.other_l_libs_entry), "");
	compiler_options_set_in_properties(co, co->props);
}

#define ASSIGN_PROPERTY_VALUE_TO_ENTRY(props, key, entry) \
	{ \
		gchar *str = prop_get ((props), (key)); \
		if (str) { \
			gtk_entry_set_text (GTK_ENTRY((entry)), str); \
			g_free (str); \
		} else { \
			gtk_entry_set_text (GTK_ENTRY((entry)), ""); \
		} \
	}

void
compiler_options_load (CompilerOptions * co, PropsID props)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *list, *node;
	gboolean valid;
	gint i;
	
	g_return_if_fail (co != NULL);

	compiler_options_clear (co);

	/* Read all available supports for the project */
	list = glist_from_data (props, "compiler.options.supports");
	node = list;
	i=0;
	while (node)
	{
		if (node->data)
		{
			model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->widgets.supp_clist));
			g_assert (model);
			valid = gtk_tree_model_get_iter_first (model, &iter);
			while (strcmp (anjuta_supports[i][ANJUTA_SUPPORT_ID],
					ANJUTA_SUPPORTS_END_STRING) != 0 && valid)
			{
				if (strcmp (anjuta_supports[i][ANJUTA_SUPPORT_ID], node->data)==0)
				{
					gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, TRUE, -1);
					break;
				}
				valid = gtk_tree_model_get_iter_first (model, &iter);
				i++;
			}
		}
		node = g_list_next (node);
	}
	glist_strings_free (list);
	
	/* For now, disable supports for projects. */
	if (app->project_dbase->project_is_open == TRUE)
	{
		gtk_widget_set_sensitive (co->widgets.supp_clist, FALSE);
	}

	CLIST_APPEND_STRING_ALL ("compiler.options.include.paths", co->widgets.inc_clist, 0);
	CLIST_APPEND_STRING_ALL ("compiler.options.library.paths", co->widgets.lib_paths_clist, 0);
	CLIST_APPEND_STRING_ALL ("compiler.options.libraries", co->widgets.lib_clist, 1);
	CLIST_UPDATE_BOOLEAN_ALL ("compiler.options.libraries.selected", co->widgets.lib_clist, 0);
	CLIST_APPEND_STRING_ALL ("compiler.options.defines", co->widgets.def_clist, 0);
	CLIST_UPDATE_BOOLEAN_ALL ("compiler.options.warning.buttons", co->widgets.warnings_clist, 0);
	
	list = glist_from_data (props, "compiler.options.optimize.buttons");
	node = list;
	i = 0;
	while (node)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(co->widgets.optimize_button[i]),
			atoi(node->data));
		node = g_list_next (node);
		i++;
	}
	glist_strings_free (list);
	
	list = glist_from_data (props, "compiler.options.other.buttons");
	node = list;
	i = 0;
	while (node) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(co->widgets.other_button[i]),
			atoi(node->data));
		node = g_list_next (node);
		i++;
	}
	glist_strings_free (list);

	ASSIGN_PROPERTY_VALUE_TO_ENTRY(props, "compiler.options.other.c.flags", co->widgets.other_c_flags_entry);
	ASSIGN_PROPERTY_VALUE_TO_ENTRY(props, "compiler.options.other.l.flags",	co->widgets.other_l_flags_entry);
	ASSIGN_PROPERTY_VALUE_TO_ENTRY(props, "compiler.options.other.l.libs", co->widgets.other_l_libs_entry);
	
	compiler_options_set_in_properties (co, co->props);
}

void
compiler_options_show (CompilerOptions * co)
{
	if (co->is_showing)
	{
		gdk_window_raise (co->widgets.window->window);
		return;
	}
	gtk_widget_set_uposition (co->widgets.window, co->win_pos_x,
				  co->win_pos_y);
	gtk_widget_show (co->widgets.window);
	co->is_showing = TRUE;
}

void
compiler_options_hide (CompilerOptions * co)
{
	if (!co)
		return;
	if (co->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (co->widgets.window->window,
				    &co->win_pos_x, &co->win_pos_y);
	co->is_showing = FALSE;
}

#define GET_ALL_STRING_DATA(widget, str, separator, col) \
{ \
	GtkTreeModel *model; \
	GtkTreeIter iter; \
	gboolean valid; \
	gchar* tmp; \
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget)); \
	g_assert(model); \
	valid = gtk_tree_model_get_iter_first(model, &iter); \
	while (valid) \
	{ \
		gchar *text; \
		gtk_tree_model_get (model, col, &text, -1); \
		tmp = g_strconcat (str, separator, text, NULL); \
		g_free (str); \
		str = tmp; \
		valid = gtk_tree_model_iter_next (model, &iter); \
	} \
}

static gchar *
get_supports (CompilerOptions *co, gint item, gchar *separator)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *str;
	gint i;
	
	str = g_strdup ("");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->widgets.supp_clist));
	g_assert(model);
	valid = gtk_tree_model_get_iter_first(model, &iter);
	i = 0;
	while (valid)
	{
		gboolean value;
		gchar *text, *tmp;
		
		gtk_tree_model_get (model, &iter, 0, &value, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		if (!value)
		{
			i++;
			continue;
		}
		text = anjuta_supports [i][item];
		tmp = g_strconcat (str, text, separator, NULL);
		g_free (str);
		str = tmp;
		i++;
	}
	return str;
}

static gchar *
get_include_paths (CompilerOptions * co, gboolean with_support)
{
	gchar *str;

	str = g_strdup ("");
	GET_ALL_STRING_DATA (co->widgets.inc_clist, str, " -I", 0);
	
	if (with_support)
	{
		gchar *supports = get_supports (co, ANJUTA_SUPPORT_FILE_CFLAGS, " ");
		gchar *tmp = g_strconcat (str, tmp, NULL);
		g_free (str);
		g_free (supports);
		str = tmp;
	}
	return str;
}

static gchar *
get_library_paths (CompilerOptions * co)
{
	gchar *str;

	str = g_strdup ("");
	GET_ALL_STRING_DATA (co->widgets.lib_paths_clist, str, " -L", 0);
	return str;
}

static gchar *
get_libraries (CompilerOptions * co, gboolean with_support)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *str, *tmp;
	gchar *text;

	str = g_strdup ("");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->widgets.lib_clist));
	g_assert(model);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gboolean value;
		
		gtk_tree_model_get (model, &iter, 0, &value, 1, &text, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		if (!value)
			continue;
		
		 /* If it starts with '*' then consider it as an object file and not a lib file */
		if (*text == '*')
			/* Remove the '*' and put the rest */
			tmp = g_strconcat (str, " ", &text[1], NULL);
		else /* Lib file */
			tmp = g_strconcat (str, " -l", text, NULL);
		g_free (str);
		str = tmp;
	}
	if (with_support)
	{
		gchar *supports = get_supports (co, ANJUTA_SUPPORT_FILE_LIBS, " ");
		gchar *tmp = g_strconcat (str, tmp, NULL);
		g_free (str);
		g_free (supports);
		str = tmp;
	}
	return str;
}

static gchar *
get_defines (CompilerOptions * co)
{
	gchar *str;
	str = g_strdup ("");
	GET_ALL_STRING_DATA (co->widgets.def_clist, str, " -D", 0);
	return str;
}

static gchar*
get_warnings(CompilerOptions *co)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *str, *tmp;
	gint i;
	
	str = g_strdup ("");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW(co->widgets.warnings_clist));
	g_assert(model);
	valid = gtk_tree_model_get_iter_first (model, &iter);
	i = 0;
	while (valid)
	{
		gboolean value;
		gtk_tree_model_get (model, &iter, 0, &value, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
		if (!value)
		{
			i += 2;
			continue;
		}
		tmp = str;
		str = g_strconcat (tmp, anjuta_warnings[i], NULL);
		g_free (tmp);
		i += 2;
	}
	return str;
}

static gchar*
get_optimization (CompilerOptions *co)
{
	gchar *str;
	gint i;
	
	str = g_strdup ("");
	for (i = 0; i < 4; i++)
	{
		gboolean state;
		gchar *tmp;
		GtkToggleButton *b;

		b= GTK_TOGGLE_BUTTON (co->widgets.optimize_button[i]);
		state = gtk_toggle_button_get_active (b);
		if (state)
		{
			tmp = str;
			str = g_strconcat (tmp, optimize_button_option[i], NULL);
			g_free (tmp);
		}
	}
	return str;
}

static gchar*
get_others (CompilerOptions *co)
{
	gchar *str;
	gint i;
	
	str = g_strdup ("");
	for (i = 0; i < 4; i++)
	{
		gboolean state;
		gchar *tmp;
		GtkToggleButton *b;
		
		b = GTK_TOGGLE_BUTTON (co->widgets.other_button[i]);
		state = gtk_toggle_button_get_active (b);
		if (state)
		{
			tmp = str;
			str = g_strconcat (tmp, other_button_option[i], NULL);
			g_free (tmp);
		}
	}
	return str;
}

void
compiler_options_set_in_properties (CompilerOptions * co, PropsID props)
{
	gchar *buff;

	buff = get_include_paths (co, TRUE);
	prop_set_with_key (props, "anjuta.compiler.includes", buff);
	g_free (buff);

	buff = get_library_paths (co);
	prop_set_with_key (props, "anjuta.linker.library.paths", buff);
	g_free (buff);

	buff = get_libraries (co, TRUE);
	prop_set_with_key (props, "anjuta.linker.libraries", buff);
	g_free (buff);

	buff = get_defines (co);
	prop_set_with_key (props, "anjuta.compiler.defines", buff);
	g_free (buff);

	buff = get_warnings(co);
	prop_set_with_key (props, "anjuta.compiler.warning.flags", buff);
	g_free (buff);
	
	
	buff = get_optimization(co);
	prop_set_with_key (props, "anjuta.compiler.optimization.flags", buff);
	g_free (buff);

	buff = get_others(co);
	prop_set_with_key (props, "anjuta.compiler.other.flags", buff);
	g_free (buff);
	
	prop_set_with_key (props, "anjuta.compiler.additional.flags",
		gtk_entry_get_text (GTK_ENTRY (co->widgets.other_c_flags_entry)));
	prop_set_with_key (props, "anjuta.linker.additional.flags",
		gtk_entry_get_text (GTK_ENTRY (co->widgets.other_l_flags_entry)));
	prop_set_with_key (props, "anjuta.linker.additional.libs",
		gtk_entry_get_text (GTK_ENTRY (co->widgets.other_l_libs_entry)));

	prop_set_with_key (props, "anjuta.compiler.flags",
			   "$(anjuta.compiler.includes) $(anjuta.compiler.defines) "
			   "$(anjuta.compiler.warning.flags) "
			   "$(anjuta.compiler.optimization.flags) "
			   "$(anjuta.compiler.other.flags) "
			   "$(anjuta.compiler.additional.flags)");

	prop_set_with_key (props, "anjuta.linker.flags",
			   "$(anjuta.linker.library.paths) $(anjuta.linker.libraries) "
			   "$(anjuta.linker.additional.flags) $(anjuta.linker.additional.libs)");
}

#define PRINT_TO_STREAM(stream, format, buff) \
	if (buff) \
	{ \
		if (strlen (buff)) \
		{ \
			fprintf (fp, "\\\n\t%s", buff); \
		} \
	}

#define PRINT_TO_STREAM_AND_FREE(stream, format, buff) \
	if (buff) \
	{ \
		if (strlen (buff)) \
		{ \
			fprintf (fp, "\\\n\t%s", buff); \
		} \
		g_free (buff); \
	}

void
compiler_options_set_prjincl_in_file (CompilerOptions* co, FILE* fp)
{
	gchar *buff;
	buff = get_include_paths (co, FALSE);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);
}

void
compiler_options_set_prjcflags_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff;
/*
	buff = get_supports (co, ANJUTA_SUPPORT_PRJ_CFLAGS, " ");
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);
*/
	buff = get_defines (co);
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);
	
	buff = (gchar*)gtk_entry_get_text (GTK_ENTRY(co->widgets.other_c_flags_entry));
	PRINT_TO_STREAM (fp, "\\\n\t%s", buff);

	buff = get_warnings(co);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);

	buff = get_optimization(co);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);

	buff = get_others(co);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);
}

void
compiler_options_set_prjlibs_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff;
	
	buff = get_libraries (co, FALSE);
	PRINT_TO_STREAM_AND_FREE(fp, "\\\n\t%s", buff);
	
	buff = (gchar*)gtk_entry_get_text (GTK_ENTRY(co->widgets.other_l_libs_entry));
	PRINT_TO_STREAM (fp, "\\\n\t%s", buff);
}

void
compiler_options_set_prjlflags_in_file (CompilerOptions * co, FILE* fp)
{
	gchar *buff;

	buff = (gchar*)gtk_entry_get_text (GTK_ENTRY(co->widgets.other_l_flags_entry));
	PRINT_TO_STREAM (fp, "\\\n\t%s", buff);
	
	buff = get_library_paths (co);
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);

/*
	buff = get_supports (co, ANJUTA_SUPPORT_PRJ_LIBS, " ");
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);
*/
}

void
compiler_options_set_prjmacros_in_file (CompilerOptions * co, FILE* fp)
{
	gchar* buff;
	
	buff = get_supports (co, ANJUTA_SUPPORT_MACROS, "\n");
	PRINT_TO_STREAM_AND_FREE (fp, "\\\n\t%s", buff);
}
