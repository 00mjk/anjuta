/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * an_symbol_view.c
 * Copyright (C) 2004 Naba Kumar
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
#include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <libgnomeui/gnome-stock-icons.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-macros.h>
#include <gdl/gdl-icons.h>
#include <libanjuta/resources.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <tm_tagmanager.h>
#include "an_symbol_view.h"
#include "an_symbol_info.h"

#define MAX_STRING_LENGTH 256

/* Pixmaps for symbol browsers */
#define ANJUTA_PIXMAP_SV_UNKNOWN          "sv_unknown.xpm"
#define ANJUTA_PIXMAP_SV_CLASS            "sv_class.xpm"
#define ANJUTA_PIXMAP_SV_FUNCTION         "sv_function.xpm"
#define ANJUTA_PIXMAP_SV_MACRO            "sv_macro.xpm"
#define ANJUTA_PIXMAP_SV_PRIVATE_FUN      "sv_private_fun.xpm"
#define ANJUTA_PIXMAP_SV_PRIVATE_VAR      "sv_private_var.xpm"
#define ANJUTA_PIXMAP_SV_PROTECTED_FUN    "sv_protected_fun.xpm"
#define ANJUTA_PIXMAP_SV_PROTECTED_VAR    "sv_protected_var.xpm"
#define ANJUTA_PIXMAP_SV_PUBLIC_FUN       "sv_public_fun.xpm"
#define ANJUTA_PIXMAP_SV_PUBLIC_VAR       "sv_public_var.xpm"
#define ANJUTA_PIXMAP_SV_STATIC_FUN       "sv_static_fun.xpm"
#define ANJUTA_PIXMAP_SV_STATIC_VAR       "sv_static_var.xpm"
#define ANJUTA_PIXMAP_SV_STRUCT           "sv_struct.xpm"
#define ANJUTA_PIXMAP_SV_VARIABLE         "sv_variable.xpm"

struct _AnjutaSymbolViewPriv
{
	TMWorkObject *tm_project;
	const TMWorkspace *tm_workspace;
	GHashTable *tm_files;
	GtkTreeModel *file_symbol_model;
};

enum
{
	PIXBUF_COLUMN,
	NAME_COLUMN,
	SVFILE_ENTRY_COLUMN,
	COLUMNS_NB
};

static GdlIcons *icon_set = NULL;
static GdkPixbuf **sv_symbol_pixbufs = NULL;
static GtkTreeViewClass *parent_class;

static void anjuta_symbol_view_open_priv (AnjutaSymbolView * sv,
										  const gchar * root_dir,
										  gboolean only_refresh);

#define CREATE_SV_ICON(N, F) \
	pix_file = anjuta_res_get_pixmap_file (F); \
	sv_symbol_pixbufs[(N)] = gdk_pixbuf_new_from_file (pix_file, NULL); \
	g_free (pix_file);

static void
sv_load_symbol_pixbufs (AnjutaSymbolView * sv)
{
	gchar *pix_file;

	if (sv_symbol_pixbufs)
		return;

	if (icon_set == NULL)
		icon_set = gdl_icons_new (16);

	sv_symbol_pixbufs = g_new (GdkPixbuf *, sv_max_t + 1);

	CREATE_SV_ICON (sv_none_t, ANJUTA_PIXMAP_SV_UNKNOWN);
	CREATE_SV_ICON (sv_class_t, ANJUTA_PIXMAP_SV_CLASS);
	CREATE_SV_ICON (sv_struct_t, ANJUTA_PIXMAP_SV_STRUCT);
	CREATE_SV_ICON (sv_union_t, ANJUTA_PIXMAP_SV_STRUCT);
	CREATE_SV_ICON (sv_typedef_t, ANJUTA_PIXMAP_SV_VARIABLE);
	CREATE_SV_ICON (sv_function_t, ANJUTA_PIXMAP_SV_FUNCTION);
	CREATE_SV_ICON (sv_variable_t, ANJUTA_PIXMAP_SV_VARIABLE);
	CREATE_SV_ICON (sv_enumerator_t, ANJUTA_PIXMAP_SV_UNKNOWN);
	CREATE_SV_ICON (sv_macro_t, ANJUTA_PIXMAP_SV_MACRO);
	CREATE_SV_ICON (sv_private_func_t, ANJUTA_PIXMAP_SV_PRIVATE_FUN);
	CREATE_SV_ICON (sv_private_var_t, ANJUTA_PIXMAP_SV_PRIVATE_VAR);
	CREATE_SV_ICON (sv_protected_func_t, ANJUTA_PIXMAP_SV_PROTECTED_FUN);
	CREATE_SV_ICON (sv_protected_var_t, ANJUTA_PIXMAP_SV_PROTECTED_VAR);
	CREATE_SV_ICON (sv_public_func_t, ANJUTA_PIXMAP_SV_PUBLIC_FUN);
	CREATE_SV_ICON (sv_public_var_t, ANJUTA_PIXMAP_SV_PUBLIC_VAR);
	sv_symbol_pixbufs[sv_cfolder_t] = gdl_icons_get_mime_icon (icon_set,
							    "application/directory-normal");
	sv_symbol_pixbufs[sv_ofolder_t] = gdl_icons_get_mime_icon (icon_set,
							    "application/directory-normal");
	sv_symbol_pixbufs[sv_max_t] = NULL;
}

/*-----------------------------------------------------------------------------
 * return the pixbufs. It will initialize pixbufs first if they weren't before
 */
GdkPixbuf *
anjuta_symbol_view_get_pixbuf (AnjutaSymbolView * sv, SVNodeType node_type)
{
	if (!sv_symbol_pixbufs)
		sv_load_symbol_pixbufs (sv);
	g_return_val_if_fail (node_type >=0 && node_type < sv_max_t, NULL);
		
	return sv_symbol_pixbufs[node_type];
}

static gboolean
on_treeview_row_search (GtkTreeModel * model, gint column,
			const gchar * key, GtkTreeIter * iter, gpointer data)
{
	DEBUG_PRINT ("Search key == '%s'", key);
	return FALSE;
}

static const AnjutaSymbolInfo *
sv_current_symbol (AnjutaSymbolView * sv)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	AnjutaSymbolInfo *info;

	g_return_val_if_fail (GTK_IS_TREE_VIEW (sv), FALSE);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sv));

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return NULL;

	gtk_tree_model_get (model, &iter, SVFILE_ENTRY_COLUMN, &info, -1);
	return info;
}

static void
on_symbol_view_row_expanded (GtkTreeView * view,
			     GtkTreeIter * iter, GtkTreePath * path)
{
}

static void
on_symbol_view_row_collapsed (GtkTreeView * view,
			      GtkTreeIter * iter, GtkTreePath * path)
{
}

static void
sv_create (AnjutaSymbolView * sv)
{
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;

	/* Tree and his model */
	store = gtk_tree_store_new (COLUMNS_NB,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, ANJUTA_TYPE_SYMBOL_INFO);

	gtk_tree_view_set_model (GTK_TREE_VIEW (sv), GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sv), TRUE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sv));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* search through the tree interactively */
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (sv), NAME_COLUMN);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (sv),
					     on_treeview_row_search,
					     NULL, NULL);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (sv), TRUE);

	g_signal_connect (G_OBJECT (sv), "row_expanded",
			  G_CALLBACK (on_symbol_view_row_expanded), sv);
	g_signal_connect (G_OBJECT (sv), "row_collapsed",
			  G_CALLBACK (on_symbol_view_row_collapsed), sv);

	g_object_unref (G_OBJECT (store));

	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column,
					 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Symbol"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
					    PIXBUF_COLUMN);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
					    NAME_COLUMN);

	gtk_tree_view_append_column (GTK_TREE_VIEW (sv), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (sv), column);

	/* sv_create_context_menu (); */
}

void
anjuta_symbol_view_clear (AnjutaSymbolView * sv)
{
	GtkTreeModel *model;
	AnjutaSymbolViewPriv* priv;
	
	priv = sv->priv;
	
	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));

	if (sv->priv->tm_project)
	{
		tm_project_save (TM_PROJECT (sv->priv->tm_project));
	}
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv));
	if (model)
	{
		/* clean out gtk_tree_store. We won't need it anymore */
		gtk_tree_store_clear (GTK_TREE_STORE (model));
	}
	if (sv->priv->file_symbol_model) {
		/* clearing file_symbol_model */
		gtk_tree_store_clear (GTK_TREE_STORE(sv->priv->file_symbol_model));
	}
	
	if (sv->priv->tm_project)
	{
		tm_project_free (sv->priv->tm_project);
		sv->priv->tm_project = NULL;
	}
}

static void
sv_assign_node_name (TMSymbol * sym, GString * s)
{
	g_assert (sym && sym->tag && s);

	g_string_assign (s, sym->tag->name);

	switch (sym->tag->type)
	{
	case tm_tag_function_t:
	case tm_tag_prototype_t:
	case tm_tag_macro_with_arg_t:
		if (sym->tag->atts.entry.arglist)
			g_string_append (s, sym->tag->atts.entry.arglist);
		break;

	default:
		{
			char *vt = sym->tag->atts.entry.var_type;
			if (vt)
			{
				if((vt = strstr(vt, "_fake_")))
				{
					char backup = *vt;
					*vt = '\0';
					g_string_append_printf (s, " [%s%s]",
											sym->tag->atts.entry.var_type,
											(sym->tag->atts.entry.isPointer ?
											 "*" : ""));
					*vt = backup;
				}
				else
				{
					g_string_append_printf (s, " [%s%s]",
											sym->tag->atts.entry.var_type,
											(sym->tag->atts.entry.isPointer ?
											 "*" : ""));
				}
			}
		}
		break;
	}
}

static void
mapping_function (GtkTreeView * treeview, GtkTreePath * path, gpointer data)
{
	gchar *str;
	GList *map = *((GList **) data);

	str = gtk_tree_path_to_string (path);
	map = g_list_append (map, str);
	*((GList **) data) = map;
};

#if 0
TMFile *
sv_get_tm_file (AnjutaSymbolView * sv, const gchar * filename)
{
	return tm_workspace_find_object (TM_WORK_OBJECT
					 (sv->priv->tm_workspace), filename,
					 TRUE);
}

#endif

void
anjuta_symbol_view_add_source (AnjutaSymbolView * sv, const gchar *filename)
{
	if (!sv->priv->tm_project)
		return;
	tm_project_add_file (TM_PROJECT (sv->priv->tm_project), filename, TRUE);
/*
	for (tmp = app->text_editor_list; tmp; tmp = g_list_next(tmp))
	{
		te = (TextEditor *) tmp->data;
		if (te && !te->tm_file && (0 == strcmp(te->full_filename, filename)))
			te->tm_file = tm_workspace_find_object(TM_WORK_OBJECT(app->tm_workspace)
			  , filename, FALSE);
	}
*/
/*	sv_populate (sv); */
}

void
anjuta_symbol_view_remove_source (AnjutaSymbolView *sv, const gchar *filename)
{
	TMWorkObject *source_file;
	
	if (!sv->priv->tm_project)
		return;

	source_file = tm_project_find_file (sv->priv->tm_project, filename, FALSE);
	if (source_file)
	{
		/* GList *node;
		TextEditor *te; */
		tm_project_remove_object (TM_PROJECT (sv->priv->tm_project),
								  source_file);
/*		for (node = app->text_editor_list; node; node = g_list_next(node))
		{
			te = (TextEditor *) node->data;
			if (te && (source_file == te->tm_file))
				te->tm_file = NULL;
		}
		sv_populate (sv);
*/
	}
/*	else
		g_warning ("Unable to find %s in project", full_fn);
*/
}

static void
anjuta_symbol_view_add_children (AnjutaSymbolView *sv, TMSymbol *sym,
								 GtkTreeStore *store,
								 GString *s, GtkTreeIter *iter)
{
	if (((iter == NULL) || (tm_tag_function_t != sym->tag->type)) &&
		(sym->info.children) && (sym->info.children->len > 0))
	{
		unsigned int j;
		SVNodeType type;
		AnjutaSymbolInfo *sfile;

		if (iter == NULL)
			DEBUG_PRINT ("Total nodes: %d", sym->info.children->len);
		
		for (j = 0; j < sym->info.children->len; j++)
		{
			TMSymbol *sym1 = TM_SYMBOL (sym->info.children->pdata[j]);
			GtkTreeIter sub_iter;
		
			// if (!sym1 || ! sym1->tag || ! sym1->tag->atts.entry.file)
			//	continue;
			
			if (!sym1 || ! sym1->tag)
				continue;
			
			type = anjuta_symbol_info_get_node_type (sym1);
			
			if (sv_none_t == type)
				continue;
			
			sv_assign_node_name (sym1, s);
			
			if (sym && sym->tag && sym->tag->atts.entry.scope)
			{
				g_string_insert (s, 0, "::");
				g_string_insert (s, 0, sym->tag->atts.entry.scope);
			}
			sfile = anjuta_symbol_info_new (sym1, type);
			
			gtk_tree_store_append (store, &sub_iter, iter);
			gtk_tree_store_set (store, &sub_iter,
								PIXBUF_COLUMN,
							    anjuta_symbol_view_get_pixbuf (sv, type),
								NAME_COLUMN, s->str,
								SVFILE_ENTRY_COLUMN, sfile,
								-1);
			anjuta_symbol_view_add_children (sv, sym1, store, s, &sub_iter);
			anjuta_symbol_info_free (sfile);
		}
	}
}

/*------------------------------------------------------------------------------
 * this function will add the symbol_tag entries on the GtkTreeStore
 */
void
anjuta_symbol_view_open (AnjutaSymbolView * sv, const gchar * root_dir)
{
	anjuta_symbol_view_open_priv (sv, root_dir, FALSE);
}

static void
anjuta_symbol_view_open_priv (AnjutaSymbolView * sv, const gchar * root_dir,
							  gboolean only_refresh)
{
	GtkTreeStore *store;
	TMSymbol *symbol_tree = NULL;
	static gboolean busy = FALSE;
	GString *s;
	GList *selected_items = NULL;
	gboolean full = TRUE;

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	if (!only_refresh)
		g_return_if_fail (root_dir != NULL);

	DEBUG_PRINT ("Populating symbol view: Loading tag database ...");

	gtk_widget_set_sensitive (GTK_WIDGET (sv), FALSE);
	
	if (busy)
		return;
	else
		busy = TRUE;

	selected_items = anjuta_symbol_view_get_node_expansion_states (sv);

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (sv)));

	if (only_refresh)
	{
		gtk_tree_store_clear (store);
	}
	else
	{
		/* make sure we clear anjuta_symbol_view from previous data */
		anjuta_symbol_view_clear (sv);
		
		sv->priv->tm_project = tm_project_new (root_dir, NULL, NULL, FALSE);
	}	
	DEBUG_PRINT ("Populating symbol view: Creating symbol tree ...");
	
	if (!full)
		goto clean_leave;

	if (!sv->priv->tm_project ||
	    !sv->priv->tm_project->tags_array ||
	    (0 == sv->priv->tm_project->tags_array->len))
		goto clean_leave;

	if (!(symbol_tree =
	     tm_symbol_tree_new (sv->priv->tm_project->tags_array)))
		goto clean_leave;

	DEBUG_PRINT ("Populating symbol view: Creating symbol view ...");
	if (!symbol_tree->info.children
	    || (0 == symbol_tree->info.children->len))
	{
		tm_symbol_tree_free (symbol_tree);
		goto clean_leave;
	}
	s = g_string_sized_new (MAX_STRING_LENGTH);
	anjuta_symbol_view_add_children (sv, symbol_tree, store, s, NULL);
	g_string_free (s, TRUE);
	
	tm_symbol_tree_free (symbol_tree);
	anjuta_symbol_view_set_node_expansion_states (sv, selected_items);

clean_leave:
	if (selected_items)
		anjuta_util_glist_strings_free (selected_items);
	busy = FALSE;
	
	gtk_widget_set_sensitive (GTK_WIDGET (sv), TRUE );
}

static void
anjuta_symbol_view_finalize (GObject * obj)
{
	AnjutaSymbolView *sv = ANJUTA_SYMBOL_VIEW (obj);
	DEBUG_PRINT ("Finalizing symbolview widget");
	
	anjuta_symbol_view_clear (sv);
	
	g_hash_table_destroy (sv->priv->tm_files);
	tm_workspace_free ((gpointer) sv->priv->tm_workspace);
	g_free (sv->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
anjuta_symbol_view_dispose (GObject * obj)
{
	AnjutaSymbolView *sv = ANJUTA_SYMBOL_VIEW (obj);
	
	/* All file symbol refs would be freed when the hash table is distroyed */
	/* if (sv->priv->file_symbol_model)
	 *	g_object_unref (sv->priv->file_symbol_model);
	 */
	sv->priv->file_symbol_model = NULL;
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

enum
{
	COL_PIX, COL_NAME, COL_LINE, N_COLS
};

static void
destroy_tm_hash_value (gpointer data)
{
	AnjutaSymbolView *sv;
	TMWorkObject *tm_file;

	sv = g_object_get_data (G_OBJECT (data), "symbol_view");
	tm_file = g_object_get_data (G_OBJECT (data), "tm_file");

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	if (tm_file)
	{
		if (tm_file->parent ==
		    TM_WORK_OBJECT (sv->priv->tm_workspace))
		{
			DEBUG_PRINT ("Removing tm_file");
			tm_workspace_remove_object (tm_file, TRUE);
		}
	}
	g_object_unref (G_OBJECT (data));
}

/* Anjuta symbol view class */
static void
anjuta_symbol_view_instance_init (GObject * obj)
{
	AnjutaSymbolView *sv;
	gchar *system_tags_path;
	
	sv = ANJUTA_SYMBOL_VIEW (obj);
	sv->priv = g_new0 (AnjutaSymbolViewPriv, 1);
	sv->priv->file_symbol_model = NULL;
	sv->priv->tm_workspace = tm_get_workspace ();
	sv->priv->tm_files = g_hash_table_new_full (g_str_hash, g_str_equal,
						    g_free,
						    destroy_tm_hash_value);
	
	system_tags_path = g_build_filename (g_get_home_dir(), ".anjuta",
										 "system-tags.cache", NULL);
	/* Load gloabal tags on gtk idle */
	if (!tm_workspace_load_global_tags (system_tags_path))
		g_warning ("Unable to load global tag file");

	/* let's create symbol_view tree and other gui stuff */
	sv_create (sv);
}

static void
anjuta_symbol_view_class_init (AnjutaSymbolViewClass * klass)
{
	AnjutaSymbolViewClass *svc;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	svc = ANJUTA_SYMBOL_VIEW_CLASS (klass);
	object_class->finalize = anjuta_symbol_view_finalize;
	object_class->dispose = anjuta_symbol_view_dispose;
}

GType
anjuta_symbol_view_get_type (void)
{
	static GType obj_type = 0;

	if (!obj_type)
	{
		static const GTypeInfo obj_info = {
			sizeof (AnjutaSymbolViewClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) anjuta_symbol_view_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,	/* class_data */
			sizeof (AnjutaSymbolViewClass),
			0,	/* n_preallocs */
			(GInstanceInitFunc) anjuta_symbol_view_instance_init,
			NULL	/* value_table */
		};
		obj_type = g_type_register_static (GTK_TYPE_TREE_VIEW,
						   "AnjutaSymbolView",
						   &obj_info, 0);
	}
	return obj_type;
}

GtkWidget *
anjuta_symbol_view_new (void)
{
	return gtk_widget_new (ANJUTA_TYPE_SYMBOL_VIEW, NULL);
}

void
anjuta_symbol_view_update (AnjutaSymbolView * sv, GList *source_files)
{
	gboolean rebuild = FALSE;
	
	if (sv->priv->tm_project)
	{
		if ((TM_PROJECT (sv->priv->tm_project)->file_list == NULL) ||
		    (TM_PROJECT (sv->priv->tm_project)->file_list->len <= 0)
		    || rebuild)
		{
			tm_project_autoscan (TM_PROJECT (sv->priv->tm_project));
		}
		else
		{
			tm_project_update (sv->priv->tm_project, TRUE, TRUE, TRUE);
			tm_project_sync (TM_PROJECT (sv->priv->tm_project), source_files);
		}
		tm_project_save (TM_PROJECT (sv->priv->tm_project));
		anjuta_symbol_view_open_priv (sv, NULL, TRUE);
	}
	/*
	 * else if (p->top_proj_dir)
	 * p->tm_project = tm_project_new (p->top_proj_dir, NULL, NULL, TRUE);
	 */
}

void
anjuta_symbol_view_save (AnjutaSymbolView * sv)
{
	tm_project_save (TM_PROJECT (sv->priv->tm_project));
}

GList *
anjuta_symbol_view_get_node_expansion_states (AnjutaSymbolView * sv)
{
	GList *map = NULL;
	gtk_tree_view_map_expanded_rows (GTK_TREE_VIEW (sv),
					 mapping_function, &map);
	return map;
}

void
anjuta_symbol_view_set_node_expansion_states (AnjutaSymbolView * sv,
											  GList * expansion_states)
{
	/* Restore expanded nodes */
	if (expansion_states)
	{
		GtkTreePath *path;
		GtkTreeModel *model;
		GList *node;
		node = expansion_states;

		model = gtk_tree_view_get_model (GTK_TREE_VIEW (sv));
		while (node)
		{
			path = gtk_tree_path_new_from_string (node->data);
			gtk_tree_view_expand_row (GTK_TREE_VIEW (sv), path,
						  FALSE);
			gtk_tree_path_free (path);
			node = g_list_next (node);
		}
	}
}

G_CONST_RETURN gchar *
anjuta_symbol_view_get_current_symbol (AnjutaSymbolView * sv)
{
	const AnjutaSymbolInfo *info;

	info = sv_current_symbol (sv);
	if (!info)
		return NULL;
	return info->sym_name;
}

gboolean
anjuta_symbol_view_get_current_symbol_def (AnjutaSymbolView * sv,
										   const gchar ** const filename,
										   gint * line)
{
	const AnjutaSymbolInfo *info;

	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (line != NULL, FALSE);

	info = sv_current_symbol (sv);
	if (!info || !info->def.name)
		return FALSE;
	*filename = info->def.name;
	*line = info->def.line;
	return TRUE;
}

gboolean
anjuta_symbol_view_get_current_symbol_decl (AnjutaSymbolView * sv,
											const gchar ** const filename,
											gint * line)
{
	const AnjutaSymbolInfo *info;

	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (line != NULL, FALSE);

	info = sv_current_symbol (sv);
	if (!info || !info->decl.name)
		return FALSE;
	*filename = info->decl.name;
	*line = info->decl.line;
	return TRUE;
}

GtkTreeModel *
anjuta_symbol_view_get_file_symbol_model (AnjutaSymbolView * sv)
{
	return sv->priv->file_symbol_model;
}

static SVNodeType
tm_tag_type_to_sv_type (TMTagType tm_type)
{
	switch (tm_type)
	{
	case tm_tag_undef_t:
		return sv_none_t;
	case tm_tag_class_t:
		return sv_class_t;
	case tm_tag_enum_t:
	case tm_tag_enumerator_t:
		return sv_struct_t;
	case tm_tag_field_t:
	case tm_tag_function_t:
		return sv_function_t;
	case tm_tag_interface_t:
		return sv_class_t;
	case tm_tag_member_t:
		return sv_private_var_t;
	case tm_tag_method_t:
		return sv_private_func_t;
	case tm_tag_namespace_t:
		return sv_class_t;
	case tm_tag_package_t:
		return sv_none_t;
	case tm_tag_prototype_t:
		return sv_function_t;
	case tm_tag_struct_t:
		return sv_struct_t;
	case tm_tag_typedef_t:
		return sv_class_t;
	case tm_tag_union_t:
		return sv_struct_t;
	case tm_tag_variable_t:
	case tm_tag_externvar_t:
		return sv_variable_t;
	case tm_tag_macro_t:
	case tm_tag_macro_with_arg_t:
		return sv_macro_t;
	default:
		return sv_none_t;
	}
}

static GtkTreeModel *
create_file_symbols_model (AnjutaSymbolView * sv, TMWorkObject * tm_file,
			   guint tag_types)
{
	GtkTreeStore *store;
	GtkTreeIter iter;

	store = gtk_tree_store_new (N_COLS, GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, G_TYPE_INT);


	g_return_val_if_fail (tm_file != NULL, NULL);

	if ((tm_file->tags_array) && (tm_file->tags_array->len > 0))
	{
		TMTag *tag;
		guint i;

		/* let's parse every tag in the array */
		for (i = 0; i < tm_file->tags_array->len; ++i)
		{
			gchar *tag_name;

			tag = TM_TAG (tm_file->tags_array->pdata[i]);
			if (tag == NULL)
				continue;
			if (tag->type & tag_types)
			{
				SVNodeType sv_type =
					tm_tag_type_to_sv_type (tag->type);

				if ((NULL != tag->atts.entry.scope)
				    && isalpha (tag->atts.entry.scope[0]))
					tag_name =
						g_strdup_printf
						("%s::%s [%ld]",
						 tag->atts.entry.scope,
						 tag->name,
						 tag->atts.entry.line);
				else
					tag_name =
						g_strdup_printf ("%s [%ld]",
								 tag->name,
								 tag->atts.
								 entry.line);
				/*
				 * Appends a new row to tree_store. If parent is non-NULL, then it will 
				 * append the new row after the last child of parent, otherwise it will append 
				 * a row to the top level. iter will be changed to point to this new row. The 
				 * row will be empty after this function is called. To fill in values, you need 
				 * to call gtk_tree_store_set() or gtk_tree_store_set_value().
				 */
				gtk_tree_store_append (store, &iter, NULL);

				/* filling the row */
				gtk_tree_store_set (store, &iter,
						    COL_PIX,
							anjuta_symbol_view_get_pixbuf (sv, sv_type),
						    COL_NAME, tag_name,
						    COL_LINE, tag->atts.entry.line, -1);
				g_free (tag_name);
			}
		}
	}
	return GTK_TREE_MODEL (store);
}

void
anjuta_symbol_view_workspace_add_file (AnjutaSymbolView * sv,
									   const gchar * file_uri)
{
	const gchar *uri;
	TMWorkObject *tm_file;
	GtkTreeModel *store = NULL;

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (file_uri != NULL);

	if (strncmp (file_uri, "file://", 7) == 0)
		uri = &file_uri[7];

	store = g_hash_table_lookup (sv->priv->tm_files, uri);
	if (!store)
	{
		DEBUG_PRINT ("Adding Symbol URI: %s", file_uri);
		tm_file =
			tm_workspace_find_object (TM_WORK_OBJECT
						  (sv->priv->tm_workspace),
						  uri, FALSE);
		if (!tm_file)
		{
			tm_file = tm_source_file_new (uri, TRUE);
			if (tm_file)
				tm_workspace_add_object (tm_file);
		}
		else
		{
			tm_source_file_update (TM_WORK_OBJECT (tm_file), TRUE,
					       FALSE, TRUE);
		}
		if (tm_file)
		{
			store = create_file_symbols_model (sv, tm_file,
							   tm_tag_max_t);
			g_object_set_data (G_OBJECT (store), "tm_file",
					   tm_file);
			g_object_set_data (G_OBJECT (store), "symbol_view",
					   sv);
			g_hash_table_insert (sv->priv->tm_files,
					     g_strdup (uri), store);
		}
	}
	sv->priv->file_symbol_model = store;
}

void
anjuta_symbol_view_workspace_remove_file (AnjutaSymbolView * sv,
										  const gchar * file_uri)
{
	const gchar *uri;

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (file_uri != NULL);

	DEBUG_PRINT ("Removing Symbol URI: %s", file_uri);
	if (strncmp (file_uri, "file://", 7) == 0)
		uri = &file_uri[7];

	if (g_hash_table_lookup (sv->priv->tm_files, uri))
		g_hash_table_remove (sv->priv->tm_files, uri);
}

void
anjuta_symbol_view_workspace_update_file (AnjutaSymbolView * sv,
										  const gchar * old_file_uri,
										  const gchar * new_file_uri)
{
	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (new_file_uri != NULL);
	if (old_file_uri)
		anjuta_symbol_view_workspace_remove_file (sv, old_file_uri);
	anjuta_symbol_view_workspace_add_file (sv, old_file_uri);
#if 0
	const gchar *uri;
	TMWorkObject *tm_file;
	GtkTreeModel *store = NULL;

	g_return_if_fail (ANJUTA_IS_SYMBOL_VIEW (sv));
	g_return_if_fail (new_file_uri != NULL);
	g_return_if_fail (strncmp (new_file_uri, "file://", 7) == 0);

	if (old_file_uri)
	{
		/* Rename old uri to new one */
		gchar *orig_key;
		gboolean success;

		g_return_if_fail (strncmp (old_file_uri, "file://", 7) == 0);

		uri = &old_file_uri[7];
		success =
			g_hash_table_lookup_extended (sv->priv->tm_files, uri,
						      (gpointer *) & orig_key,
						      (gpointer *) & store);
		if (success)
		{
			if (strcmp (old_file_uri, new_file_uri) != 0)
			{
				DEBUG_PRINT ("Renaming Symbol URI: %s to %s",
					     old_file_uri, new_file_uri);
				g_hash_table_steal (sv->priv->tm_files, uri);
				g_free (orig_key);
				uri = &new_file_uri[7];
				g_hash_table_insert (sv->priv->tm_files,
						     g_strdup (uri), store);
			}
			/* Update tm_file */
			tm_file =
				g_object_get_data (G_OBJECT (store),
						   "tm_file");
			g_assert (tm_file != NULL);
			tm_source_file_update (TM_WORK_OBJECT (tm_file), TRUE,
					       FALSE, TRUE);
		}
		else
		{
			/* Old uri not found. Just add the new one. */
			anjuta_symbol_view_workspace_add_file (sv,
							       new_file_uri);
		}
	}
	else
	{
		/* No old uri to rename. Just add the new one. */
		anjuta_symbol_view_workspace_add_file (sv, new_file_uri);
	}
#endif
}

gint
anjuta_symbol_view_workspace_get_line (AnjutaSymbolView * sv,
									   GtkTreeIter * iter)
{
	g_return_val_if_fail (iter != NULL, -1);
	if (sv->priv->file_symbol_model)
	{
		gint line;
		gtk_tree_model_get (GTK_TREE_MODEL
				    (sv->priv->file_symbol_model), iter,
				    COL_LINE, &line, -1);
		return line;
	}
	return -1;
}

#define IS_DECLARATION(T) ((tm_tag_prototype_t == (T)) || (tm_tag_externvar_t == (T)) \
  || (tm_tag_typedef_t == (T)))

gboolean
anjuta_symbol_view_get_file_symbol (AnjutaSymbolView * sv,
				    const gchar * symbol,
				    gboolean prefer_definition,
				    const gchar ** const filename,
				    gint * line)
{
	TMWorkObject *tm_file;
	GPtrArray *tags;
	guint i;
	int cmp;
	TMTag *tag = NULL, *local_tag = NULL, *global_tag = NULL;
	TMTag *local_proto = NULL, *global_proto = NULL;

	g_return_val_if_fail (symbol != NULL, FALSE);

	/* Get the matching definition and declaration in the local file */
	if (sv->priv->file_symbol_model != NULL)
	{
		tm_file =
			g_object_get_data (G_OBJECT
					   (sv->priv->file_symbol_model),
					   "tm_file");
		if (tm_file && tm_file->tags_array
		    && tm_file->tags_array->len > 0)
		{
			for (i = 0; i < tm_file->tags_array->len; ++i)
			{
				tag = TM_TAG (tm_file->tags_array->pdata[i]);
				cmp = strcmp (symbol, tag->name);
				if (0 == cmp)
				{
					if (IS_DECLARATION (tag->type))
						local_proto = tag;
					else
						local_tag = tag;
				}
				else if (cmp < 0)
					break;
			}
		}
	}
	/* Get the matching definition and declaration in the workspace */
	if (!(((prefer_definition) && (local_tag)) ||
	      ((!prefer_definition) && (local_proto))))
	{
		tags = TM_WORK_OBJECT (tm_get_workspace ())->tags_array;
		if (tags && (tags->len > 0))
		{
			for (i = 0; i < tags->len; ++i)
			{
				tag = TM_TAG (tags->pdata[i]);
				if (tag->atts.entry.file)
				{
					cmp = strcmp (symbol, tag->name);
					if (cmp == 0)
					{
						if (IS_DECLARATION
						    (tag->type))
							global_proto = tag;
						else
							global_tag = tag;
					}
					else if (cmp < 0)
						break;
				}
			}
		}
	}
	if (prefer_definition)
	{
		if (local_tag)
			tag = local_tag;
		else if (global_tag)
			tag = global_tag;
		else if (local_proto)
			tag = local_proto;
		else
			tag = global_proto;
	}
	else
	{
		if (local_proto)
			tag = local_proto;
		else if (global_proto)
			tag = global_proto;
		else if (local_tag)
			tag = local_tag;
		else
			tag = global_tag;
	}

	if (tag)
	{
		*filename =
			g_strdup (tag->atts.entry.file->work_object.
				  file_name);
		*line = tag->atts.entry.line;
		return TRUE;
	}
	return FALSE;
}
