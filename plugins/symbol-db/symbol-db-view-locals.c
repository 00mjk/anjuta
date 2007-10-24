/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) Massimo Cora' 2007 <maxcvs@email.it>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>
#include <libegg/menu/egg-combo-action.h>

#include "symbol-db-view-locals.h"
#include "symbol-db-engine.h"
#include "symbol-db-engine-iterator.h"
#include "symbol-db-engine-iterator-node.h"
#include "symbol-db-view.h"


enum {
	COLUMN_PIXBUF,
	COLUMN_NAME,
	COLUMN_SYMBOL_ID,
	COLUMN_MAX
};

static GtkTreeViewClass *parent_class = NULL;

struct _SymbolDBViewLocalsPriv
{
	gchar *current_file;
	gint insert_handler;
	gint update_handler;
	gint remove_handler;	
	gint scan_end_handler;
	
	GTree *nodes_displayed;
	GTree *waiting_for;	
	
	SymbolDBEngine *dbe;
};

typedef struct _WaitingForSymbol {
	gint child_symbol_id;
	gchar *child_symbol_name;
	const GdkPixbuf *pixbuf;
	
} WaitingForSymbol;

typedef struct _TraverseData {
	SymbolDBViewLocals *dbvl;
	SymbolDBEngine *dbe;

} TraverseData;

static void
trigger_on_symbol_inserted (SymbolDBViewLocals *dbvl, gint symbol_id);


static gint
gtree_compare_func (gconstpointer a, gconstpointer b, gpointer user_data)
{
	return (gint)a - (gint)b;
}

static void
waiting_for_symbol_destroy (WaitingForSymbol *wfs)
{
	g_return_if_fail (wfs != NULL);
	g_free (wfs->child_symbol_name);
	g_free (wfs);
}


static void
sdb_view_locals_init (SymbolDBViewLocals *dbvl)
{
	DEBUG_PRINT ("sdb_view_locals_init  ()");
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeStore *store;
	SymbolDBViewLocalsPriv *priv;
	
	g_return_if_fail (dbvl != NULL);
	
	dbvl->priv = g_new0 (SymbolDBViewLocalsPriv, 1);		
	priv = dbvl->priv;
	
	priv->dbe = NULL;
	priv->current_file = NULL;
	priv->insert_handler = -1;
	priv->update_handler = -1;
	priv->remove_handler = -1;
	priv->nodes_displayed = NULL;
	priv->waiting_for = NULL;
	
	store = gtk_tree_store_new (COLUMN_MAX, GDK_TYPE_PIXBUF,
				    G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);

	gtk_tree_view_set_model (GTK_TREE_VIEW (dbvl), GTK_TREE_MODEL (store));	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dbvl), FALSE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dbvl));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	/* search through the tree interactively */
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (dbvl), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (dbvl), COLUMN_NAME);
	
	/* Columns */
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (column,
					 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Symbol"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
					    COLUMN_PIXBUF);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
					    COLUMN_NAME);

	gtk_tree_view_append_column (GTK_TREE_VIEW (dbvl), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (dbvl), column);	
	
	/* gtk 2.12 
	 * gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (dbvl), FALSE); */
}

static gboolean
traverse_free_waiting_for (gpointer key, gpointer value, gpointer data)
{
	waiting_for_symbol_destroy ((WaitingForSymbol *)value);
	return FALSE;
}

static void
sdb_view_locals_finalize (GObject *object)
{
	SymbolDBViewLocals *locals = SYMBOL_DB_VIEW_LOCALS (object);
	SymbolDBViewLocalsPriv *priv = locals->priv;

	DEBUG_PRINT ("finalizing symbol_db_view_locals ()");
	
	g_free (priv->current_file);
	priv->current_file = NULL;
	
	if (priv->nodes_displayed)
		g_tree_destroy (priv->nodes_displayed);
	
	/* free the waiting_for structs before destroying the tree itself */
	if (priv->waiting_for)
	{
		g_tree_foreach (priv->waiting_for, traverse_free_waiting_for, NULL);
		g_tree_destroy (priv->waiting_for);
	}
	
	if (priv->dbe) 
	{
		if (priv->insert_handler > 0)
			g_signal_handler_disconnect (priv->dbe, priv->insert_handler);

		if (priv->remove_handler > 0)
			g_signal_handler_disconnect (priv->dbe, priv->remove_handler);

		if (priv->scan_end_handler > 0)
			g_signal_handler_disconnect (priv->dbe, priv->scan_end_handler);
	}
	
	g_free (priv);
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
sdb_view_locals_class_init (SymbolDBViewLocalsClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = sdb_view_locals_finalize;
}

GType
symbol_db_view_locals_get_type (void) 
{
	static GType our_type = 0;

	if(our_type == 0)
	{
		static const GTypeInfo our_info =
		{
			sizeof (SymbolDBViewLocalsClass), /* class_size */
			(GBaseInitFunc) NULL, /* base_init */
			(GBaseFinalizeFunc) NULL, /* base_finalize */
			(GClassInitFunc) sdb_view_locals_class_init, /* class_init */
			(GClassFinalizeFunc) NULL, /* class_finalize */
			NULL /* class_data */,
			sizeof (SymbolDBViewLocals), /* instance_size */
			0, /* n_preallocs */
			(GInstanceInitFunc) sdb_view_locals_init, /* instance_init */
			NULL /* value_table */
		};

		our_type = g_type_register_static (GTK_TYPE_TREE_VIEW, "SymbolDBViewLocals",
		                                   &our_info, 0);
	}

	return our_type;
}

GtkWidget *
symbol_db_view_locals_new (void)
{
	return GTK_WIDGET (g_object_new (SYMBOL_TYPE_DB_VIEW_LOCALS, NULL));
}

static GtkTreeRowReference *
do_add_root_symbol_to_view (SymbolDBViewLocals *dbvl, const GdkPixbuf *pixbuf, 
							const gchar* symbol_name, gint symbol_id)
{
	SymbolDBViewLocalsPriv *priv;
	GtkTreeStore *store;
	GtkTreeIter child_iter;
	GtkTreePath *path;
	GtkTreeRowReference *row_ref;
	
	g_return_val_if_fail (dbvl != NULL, NULL);
	
	priv = dbvl->priv;	
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));
 	
	gtk_tree_store_append (store, &child_iter, NULL);
			
	gtk_tree_store_set (store, &child_iter,
		COLUMN_PIXBUF, pixbuf,
		COLUMN_NAME, symbol_name,
		COLUMN_SYMBOL_ID, symbol_id,
		-1);	

	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)),
                                          &child_iter);	
	row_ref = gtk_tree_row_reference_new (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)), path);
	gtk_tree_path_free (path);
	
	return row_ref;
}

static GtkTreeRowReference *
do_add_child_symbol_to_view (SymbolDBViewLocals *dbvl, gint parent_symbol_id,
					   const GdkPixbuf *pixbuf, const gchar* symbol_name,
					   gint symbol_id)
{
	SymbolDBViewLocalsPriv *priv;
	GtkTreePath *path;
	GtkTreeStore *store;
	GtkTreeIter iter, child_iter;
	GtkTreeRowReference *row_ref;
	
	g_return_val_if_fail (dbvl != NULL, NULL);
	
	priv = dbvl->priv;	
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));
	
	/* look into nodes_displayed g_tree for the gtktreepath of the parent iter,
	 * get the gtktreeiter, and add a child 
	 */
	row_ref = g_tree_lookup (priv->nodes_displayed, (gpointer)parent_symbol_id);
	path = gtk_tree_row_reference_get_path (row_ref);
	
	if (path == NULL) {
		DEBUG_PRINT ("do_add_symbol_to_view (): something went wrong.");
		return NULL;		
	}
	
	if (gtk_tree_model_get_iter (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)),
                                 &iter, path) == FALSE) {									 
		DEBUG_PRINT ("do_add_symbol_to_view (): iter was not set ?![%s %d] parent %d",
					 symbol_name, symbol_id, parent_symbol_id);
		return NULL;
	}

	gtk_tree_path_free (path);
	
	gtk_tree_store_append (store, &child_iter, &iter);
			
	gtk_tree_store_set (store, &child_iter,
		COLUMN_PIXBUF, pixbuf,
		COLUMN_NAME, symbol_name,
		COLUMN_SYMBOL_ID, symbol_id,
		-1);	
	
	gchar *tmp_str = gtk_tree_path_to_string (
					gtk_tree_model_get_path (
						gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)),
                                          &child_iter));
/*	DEBUG_PRINT ("do_add_symbol_to_view (): added name: %s, id: %d, path: %s",
				 symbol_name,
				 symbol_id,
				 tmp_str);*/
	g_free (tmp_str);
	
	path = gtk_tree_model_get_path (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)),
                                          &child_iter);
	row_ref = gtk_tree_row_reference_new (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)), 
										  path);
	gtk_tree_path_free (path);
	
	return row_ref;
}

static gboolean 
traverse_on_scan_end (gpointer key, gpointer value, gpointer data)
{
	TraverseData *tdata;
	SymbolDBEngine *dbe;
	SymbolDBViewLocals *dbvl;
	SymbolDBViewLocalsPriv *priv;
	SymbolDBEngineIterator *iterator;
	gint parent_id;

	g_return_val_if_fail (data != NULL, FALSE);

	tdata = (TraverseData *)data;

	dbe = tdata->dbe;
	dbvl = tdata->dbvl;

	g_return_val_if_fail (dbe != NULL, FALSE);
	g_return_val_if_fail (dbvl != NULL, FALSE);

	priv = dbvl->priv;

	parent_id = (gint) key;
	DEBUG_PRINT ("traverse_on_scan_end (): something has been left on "
				"waiting_for_tree.. checking for %d", parent_id);

	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, parent_id, 
													   SYMINFO_SIMPLE |
													   SYMINFO_ACCESS |
													   SYMINFO_KIND);	
	if (iterator != NULL) 
	{
		SymbolDBEngineIteratorNode *iter_node;
		const GdkPixbuf *pixbuf;
		const gchar* symbol_name;
		GtkTreeRowReference *row_ref;
		
		iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
		
		pixbuf = symbol_db_view_get_pixbuf (
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_KIND),
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_ACCESS));
		symbol_name = symbol_db_engine_iterator_node_get_symbol_name (iter_node);
		
		row_ref = do_add_root_symbol_to_view (dbvl, pixbuf, 
							symbol_name, parent_id);

		g_tree_insert (priv->nodes_displayed, (gpointer)parent_id, 
				   row_ref);

		/* now the waiters should be added as children */
		trigger_on_symbol_inserted  (dbvl, parent_id);
	}
	
	/* continue the traversing */
	return FALSE;
}


static void
on_scan_end (SymbolDBEngine *dbe, gpointer data)
{
	SymbolDBViewLocals *dbvl;
	SymbolDBViewLocalsPriv *priv;
	gint waiting_for_size;
	TraverseData tdata;

	dbvl = SYMBOL_DB_VIEW_LOCALS (data);
	g_return_if_fail (dbvl != NULL);	
	priv = dbvl->priv;

	/* ok, symbol parsing has ended, are we sure that all the waiting_for
	 * objects have been checked?
	 * If it's not the case then try to add it to the on the root of the gtktreeview
	 * and to trigger the insertion.
	 */
	if ((waiting_for_size = g_tree_nnodes (priv->waiting_for)) <= 0)
		return;

	/* we have something left. Search the parent_symbol_id [identified by the
	 * waiting_for id]
	 */
	tdata.dbvl = dbvl;
	tdata.dbe = dbe;

	g_tree_foreach (priv->waiting_for, traverse_on_scan_end, &tdata);
}

static void
do_recurse_subtree_and_remove (SymbolDBViewLocals *dbvl, 
							   GtkTreeIter *parent_subtree_iter)
{
	gint curr_symbol_id;
	const GdkPixbuf *curr_pixbuf;
	GtkTreeStore *store;
	gchar *curr_symbol_name;

	SymbolDBViewLocalsPriv *priv;
	
	g_return_if_fail (dbvl != NULL);
	
	priv = dbvl->priv;
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));	
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), parent_subtree_iter,
				COLUMN_SYMBOL_ID, &curr_symbol_id,
			    COLUMN_PIXBUF, &curr_pixbuf, 
				COLUMN_NAME, &curr_symbol_name,	/* no strdup required */
				-1);
	
	/*DEBUG_PRINT ("do_recurse_subtree_and_remove (): curr_symbol_id %d", 
				 curr_symbol_id);*/
				 
	while (gtk_tree_model_iter_has_child  (GTK_TREE_MODEL (store), parent_subtree_iter)) 
	{
		GtkTreeIter child;
		gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, parent_subtree_iter);
		
		/* recurse */
		do_recurse_subtree_and_remove (dbvl, &child);
	}

	gtk_tree_store_remove (store, parent_subtree_iter);
	g_tree_remove (priv->nodes_displayed, (gpointer) curr_symbol_id);

	/* don't forget to free this gchar */				   
	g_free (curr_symbol_name);
}


static void 
on_symbol_removed (SymbolDBEngine *dbe, gint symbol_id, gpointer data)
{
	GtkTreeStore *store;
	SymbolDBViewLocals *dbvl;
	SymbolDBViewLocalsPriv *priv;
    GtkTreeIter  iter;	
	GtkTreeRowReference *row_ref;
	GtkTreePath *path;
	
	dbvl = SYMBOL_DB_VIEW_LOCALS (data);

	g_return_if_fail (dbvl != NULL);
	priv = dbvl->priv;

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));		

	/*DEBUG_PRINT ("on_symbol_removed (): -local- %d", symbol_id);		*/

	row_ref = g_tree_lookup (priv->nodes_displayed, (gpointer)symbol_id);
	if (row_ref == NULL) 
	{
		/*DEBUG_PRINT ("on_symbol_removed (): ERROR: cannot remove %d", symbol_id);*/
		return;
	}
 
	path = gtk_tree_row_reference_get_path (row_ref);
	if (path == NULL) 
	{
		/*DEBUG_PRINT ("on_symbol_removed (): ERROR2: cannot remove %d", symbol_id);*/
		return;
	}
	
	if (gtk_tree_model_get_iter (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)),
                                 &iter, path) == FALSE) 
	{
		DEBUG_PRINT ("on_symbol_removed (): iter was not set ?![%d]",
					 symbol_id);
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	do_recurse_subtree_and_remove (dbvl, &iter);
}


static void
add_waiting_for_symbol_to_view (SymbolDBViewLocals *dbvl, WaitingForSymbol *wfs,
								gint parent_symbol_id)
{
	SymbolDBViewLocalsPriv *priv;
	gint symbol_id_added;
	GtkTreeRowReference *child_tree_row_ref;
	
	g_return_if_fail (dbvl != NULL);
	g_return_if_fail (wfs != NULL);
	
	priv = dbvl->priv;	

	child_tree_row_ref = do_add_child_symbol_to_view (dbvl, parent_symbol_id,
					   wfs->pixbuf, wfs->child_symbol_name, wfs->child_symbol_id);
			
	symbol_id_added = wfs->child_symbol_id;
	
	/* add a new entry on gtree 'nodes_displayed' */
	g_tree_insert (priv->nodes_displayed, (gpointer)wfs->child_symbol_id, 
				   child_tree_row_ref);	
	
	/* and now trigger the inserted symbol... (recursive function). */
	if (wfs->child_symbol_id != parent_symbol_id)
		trigger_on_symbol_inserted  (dbvl, wfs->child_symbol_id);
}

static void
trigger_on_symbol_inserted (SymbolDBViewLocals *dbvl, gint symbol_id)
{
	SymbolDBViewLocalsPriv *priv;
	GSList *slist;
	WaitingForSymbol *wfs;
	
	g_return_if_fail (dbvl != NULL);
	
	priv = dbvl->priv;	

	/*DEBUG_PRINT ("trigger_on_symbol_inserted (): triggering %d", symbol_id);*/
	
	/* try to find a waiting for symbol */
	slist = g_tree_lookup (priv->waiting_for, (gpointer)symbol_id);
	
	if (slist == NULL) 
	{
		/* nothing waiting for us */
		/*DEBUG_PRINT ("trigger_on_symbol_inserted (): no children waiting for us...");*/
		return;
	}
	else {
		gint i;
		gint length = g_slist_length (slist);

/*		DEBUG_PRINT ("trigger_on_symbol_inserted (): consuming slist for parent %d",
					 symbol_id);
		DEBUG_PRINT ("trigger_on_symbol_inserted (): length is %d", length);*/
		for (i=0; i < length-1; i++)
		{
			wfs = g_slist_nth_data (slist, 0);
				
			slist = g_slist_remove (slist, wfs);

			add_waiting_for_symbol_to_view (dbvl, wfs, symbol_id);

			/* destroy the data structure */
			waiting_for_symbol_destroy (wfs);
		}
		
		/* remove the waiting for key/value */
		g_tree_remove (priv->waiting_for, (gpointer)symbol_id);		
		g_slist_free (slist);
	}
}

static void
add_new_waiting_for (SymbolDBViewLocals *dbvl, gint parent_symbol_id, 
					 const gchar* symbol_name, 
					 gint symbol_id, const GdkPixbuf *pixbuf)
{
	SymbolDBViewLocalsPriv *priv;
	gpointer node;
	
	g_return_if_fail (dbvl != NULL);	
	priv = dbvl->priv;

	/* check if we already have some children waiting for a 
	 * specific father to be inserted, then add this symbol_id to the list 
	 * (or create a new one)
	 */
	WaitingForSymbol *wfs;
			
	wfs = g_new0 (WaitingForSymbol, 1);
	wfs->child_symbol_id = symbol_id;
	wfs->child_symbol_name = g_strdup (symbol_name);
	wfs->pixbuf = pixbuf;
				
	/*DEBUG_PRINT ("add_new_waiting_for (): looking up waiting_for %d", 
				 parent_symbol_id);*/
	node = g_tree_lookup (priv->waiting_for, (gpointer)parent_symbol_id);
	if (node == NULL) 
	{
		/* no lists already set. Create one. */
		GSList *slist;					
		slist = g_slist_alloc ();			
				
		slist = g_slist_prepend (slist, wfs);
					
		/*DEBUG_PRINT ("add_new_waiting_for (): NEW adding to "
					 "waiting_for [%d]", parent_symbol_id);*/
				
		/* add it to the binary tree. */
		g_tree_insert (priv->waiting_for, (gpointer)parent_symbol_id, 
							   slist);
	}
	else 
	{
		/* found a list */
		GSList *slist;
		slist = (GSList*)node;
		
		/*DEBUG_PRINT ("prepare_for_adding (): NEW adding to "
					 "parent_waiting_for_list [%d] %s",
				 	parent_symbol_id, symbol_name);*/
		slist = g_slist_prepend (slist, wfs);
				
		g_tree_replace (priv->waiting_for, (gpointer)parent_symbol_id, 
						slist);
	}	
}

/* Put every GtkTreeView node of the subtree headed by 'parent_subtree_iter'
 * into a waiting_for GTree.
 * It's a recursive function.
 */
static void
do_recurse_subtree_and_invalidate (SymbolDBViewLocals *dbvl, 
								   GtkTreeIter *parent_subtree_iter,
								   gint parent_id_to_wait_for)
{
	gint curr_symbol_id;
	const GdkPixbuf *curr_pixbuf;
	GtkTreeStore *store;
	gchar *curr_symbol_name;

	SymbolDBViewLocalsPriv *priv;
	
	g_return_if_fail (dbvl != NULL);
	
	priv = dbvl->priv;
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));	
	
	gtk_tree_model_get (GTK_TREE_MODEL (store), parent_subtree_iter,
				COLUMN_SYMBOL_ID, &curr_symbol_id,
			    COLUMN_PIXBUF, &curr_pixbuf, 
				COLUMN_NAME, &curr_symbol_name,	/* no strdup required */
				-1);
	
	 /*DEBUG_PRINT ("do_recurse_subtree_and_invalidate (): curr_symbol_id %d,"
				"parent_id_to_wait_for %d", curr_symbol_id, parent_id_to_wait_for);*/
				 
	while (gtk_tree_model_iter_has_child  (GTK_TREE_MODEL (store), 
										   parent_subtree_iter)) 
	{
		GtkTreeIter child;
		gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &child, 
									  parent_subtree_iter);
		
		/* recurse */
		do_recurse_subtree_and_invalidate (dbvl, &child, curr_symbol_id);
	}

	/* add to waiting for */
	add_new_waiting_for (dbvl, parent_id_to_wait_for, curr_symbol_name,
						 curr_symbol_id, curr_pixbuf);
	
	gtk_tree_store_remove (store, parent_subtree_iter);
	g_tree_remove (priv->nodes_displayed, (gpointer) curr_symbol_id);

	/* don't forget to free this gchar */				   
	g_free (curr_symbol_name);
}


/* Add promptly a symbol to the gtktreeview or add it for a later add (waiting
 * for trigger).
 */
static void
prepare_for_adding (SymbolDBViewLocals *dbvl, gint parent_symbol_id, 
					const gchar* symbol_name, gint symbol_id,
					const GdkPixbuf *pixbuf)
{
	SymbolDBViewLocalsPriv *priv;
	
	g_return_if_fail (dbvl != NULL);	
	priv = dbvl->priv;
	
	/* add to root if parent_symbol_id is <= 0 */
	if (parent_symbol_id <= 0)
	{			
		GtkTreeRowReference *curr_tree_row_ref;
		/*DEBUG_PRINT ("prepare_for_adding(): parent_symbol_id <= 0 root with id [%d]",
					 symbol_id);*/
		
		/* get the current iter row reference in the just added root gtktreeview 
		 * node 
		 */
		curr_tree_row_ref = do_add_root_symbol_to_view (dbvl, pixbuf, symbol_name,
													 symbol_id);
		
		/* we'll fake the gpointer to store an int */
		g_tree_insert (priv->nodes_displayed, (gpointer)symbol_id, 
					   curr_tree_row_ref);
		
		/* let's trigger the insertion of the symbol_id, there may be some children
		 * waiting for it.
		 */
		trigger_on_symbol_inserted (dbvl, symbol_id);
	}
	else 
	{
		gpointer node;
		/* do we already have that parent_symbol displayed in gtktreeview? 
		 * If that's the case add it as children.
		 */
		node = g_tree_lookup (priv->nodes_displayed, (gpointer)parent_symbol_id);
		
		if (node != NULL) 
		{
			/* hey we found it */
			GtkTreeRowReference *child_row_ref;
			/*DEBUG_PRINT ("prepare_for_adding(): found node already displayed %d",
						 parent_symbol_id);*/
			
			child_row_ref = do_add_child_symbol_to_view (dbvl, parent_symbol_id,
				   pixbuf, symbol_name, symbol_id);
			
			/* add the children_path to the GTree. */
			g_tree_insert (priv->nodes_displayed, (gpointer)symbol_id, 
						   child_row_ref);
			trigger_on_symbol_inserted (dbvl, symbol_id);
		}
		else 
		{
			/* add it to the waiting_for trigger list */
			add_new_waiting_for (dbvl, parent_symbol_id, symbol_name, symbol_id, 
								 pixbuf);
		}
	}
}

static void 
on_symbol_inserted (SymbolDBEngine *dbe, gint symbol_id, gpointer data)
{
	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;
	SymbolDBViewLocals *dbvl;
	SymbolDBViewLocalsPriv *priv;
	
	/* it's not obligatory referred to a class inheritance */
	gint parent_symbol_id;
	
	dbvl = SYMBOL_DB_VIEW_LOCALS (data);

	g_return_if_fail (dbvl != NULL);	
	priv = dbvl->priv;	
	
	/*DEBUG_PRINT ("on_symbol_inserted (): -local- %d", symbol_id);*/
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));
	
	/* again we use a little trick to insert symbols here. First of all forget chars
	 * and symbol names. They are too much cpu-intensive. We'll use symbol-ids instead.
	 *
	 * Suppose we have a symbol_id X to insert. Where should we put it into the
	 * gtktree? Well.. look at its parent! By knowing its parent we're able to
	 * know the right place where to store this child, being it on the root /
	 * or under some path. Please note this: the whole path isn't computed at once
	 * when the global gtk tree view is loaded, but it's incremental. So we can
	 * have a case where our symbol X has a parent Y, but that parent isn't already
	 * mapped into the gtktreestore: we'll just avoid to insert 'visually' the 
	 * symbol.	 
	 *
	 */
	parent_symbol_id = symbol_db_engine_get_parent_scope_id_by_symbol_id (dbe, 
																	symbol_id);
	
	/* get the original symbol infos */
	iterator = symbol_db_engine_get_symbol_info_by_id (dbe, symbol_id, 
													   SYMINFO_SIMPLE |
													   SYMINFO_FILE_PATH |
													   SYMINFO_ACCESS |
													   SYMINFO_KIND);	
	
	if (iterator != NULL) 
	{
		SymbolDBEngineIteratorNode *iter_node;
		const GdkPixbuf *pixbuf;
		const gchar* symbol_name;
		
		iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
		
		pixbuf = symbol_db_view_get_pixbuf (
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_KIND),
						symbol_db_engine_iterator_node_get_symbol_extra_string (
							iter_node, SYMINFO_ACCESS));
		symbol_name = symbol_db_engine_iterator_node_get_symbol_name (iter_node);
		
		/* check if one of the children [if they exist] of symbol_id are already 
		 * displayed. In that case we'll invalidate all of them.
		 * i.e. we're in an updating insertion.
		 */
		SymbolDBEngineIterator *iterator_for_children;
		iterator_for_children = 
			symbol_db_engine_get_scope_members_by_symbol_id (dbe, symbol_id, 
															 SYMINFO_SIMPLE);
		if (iterator_for_children == NULL) 
		{
			/* we don't have children */
			/*DEBUG_PRINT ("on_symbol_inserted (): %d has no children.", symbol_id);*/
		}
		else 
		{
			/* hey there are some children here.. kill 'em all and put them on
			 * a waiting_for list 
			 */			
			do
			{
				gint curr_child_id;
				GtkTreeIter child_iter;
				GtkTreeRowReference *row_ref;
				GtkTreePath *path;
				SymbolDBEngineIteratorNode *iter_node;

				iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator_for_children);

				curr_child_id = 
					symbol_db_engine_iterator_node_get_symbol_id (iter_node);

				/*DEBUG_PRINT ("on_symbol_inserted (): %d has child %d",
							 symbol_id, curr_child_id);*/
				row_ref = g_tree_lookup (priv->nodes_displayed,
										 (gpointer)curr_child_id);

				if (row_ref == NULL) 
				{
					/* no node displayed found */
					continue;
				}
				
				path = gtk_tree_row_reference_get_path (row_ref);
				if (path == NULL) 
				{
					DEBUG_PRINT ("on_symbol_inserted (): path is null, something "
								 "went wrong ?!");
					continue;
				}
		
				if (gtk_tree_model_get_iter (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)),
                                 &child_iter, path) == FALSE) 
				{
					gtk_tree_path_free (path);
					continue;		
				}
				gtk_tree_path_free (path);
				
				/* put on waiting_for the subtree */
				do_recurse_subtree_and_invalidate (dbvl, &child_iter, symbol_id);
			} while (symbol_db_engine_iterator_move_next (iterator_for_children) 
					 == TRUE);
			
			g_object_unref (iterator_for_children);
		}		
		
		prepare_for_adding (dbvl, parent_symbol_id, symbol_name, symbol_id, pixbuf);
		
		g_object_unref (iterator);
	}	
	
	gtk_tree_view_expand_all (GTK_TREE_VIEW (dbvl));
}

gint
symbol_db_view_locals_get_line (SymbolDBViewLocals *dbvl,
								SymbolDBEngine *dbe,
								GtkTreeIter * iter) 
{
	GtkTreeStore *store;
		
	g_return_val_if_fail (dbvl != NULL, -1);
	g_return_val_if_fail (dbe != NULL, -1);	
	g_return_val_if_fail (iter != NULL, -1);
	
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));	
	
	if (store)
	{
		gint symbol_id;
		gint line;
		SymbolDBEngineIteratorNode *node;
		
		gtk_tree_model_get (GTK_TREE_MODEL
				    (store), iter,
				    COLUMN_SYMBOL_ID, &symbol_id, -1);
		
		/* getting line at click time with a query is faster than updating every 
		 * entry in the gtktreeview. We can be sure that the db is in a consistent 
		 * state and has all the last infos */
		node = SYMBOL_DB_ENGINE_ITERATOR_NODE (
					symbol_db_engine_get_symbol_info_by_id (dbe, symbol_id, 
															SYMINFO_SIMPLE));
		if (node != NULL) 
		{
			line = symbol_db_engine_iterator_node_get_symbol_file_pos (node);
			return line;
		}
	}
	return -1;
}								

void
symbol_db_view_locals_update_list (SymbolDBViewLocals *dbvl, SymbolDBEngine *dbe,
							  const gchar* filepath)
{
	SymbolDBViewLocalsPriv *priv;

	SymbolDBEngineIterator *iterator;
	GtkTreeStore *store;

	g_return_if_fail (dbvl != NULL);
	g_return_if_fail (filepath != NULL);
	g_return_if_fail (dbe != NULL);
		
	/*DEBUG_PRINT ("symbol_db_view_locals_update_list  () for %s", filepath);*/
	
	priv = dbvl->priv;
	g_free (priv->current_file);		

	priv->current_file = g_strdup (filepath);	
	priv->nodes_displayed = g_tree_new_full ((GCompareDataFunc)&gtree_compare_func, 
										 NULL,
										 NULL,
										 (GDestroyNotify)&gtk_tree_row_reference_free);

	priv->waiting_for = g_tree_new_full ((GCompareDataFunc)&gtree_compare_func, 
									 NULL,
									 NULL,
									 NULL);


	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dbvl)));	
	
	/* Removes all rows from tree_store */
	gtk_tree_store_clear (store);

	iterator = symbol_db_engine_get_file_symbols (dbe, filepath, SYMINFO_SIMPLE |
												  	SYMINFO_ACCESS |
													SYMINFO_KIND);		
	
	if (iterator != NULL)
	{
		do {
			gint curr_symbol_id;
			SymbolDBEngineIteratorNode *iter_node;

			iter_node = SYMBOL_DB_ENGINE_ITERATOR_NODE (iterator);
			
			curr_symbol_id = symbol_db_engine_iterator_node_get_symbol_id (iter_node);

			/* we can just call the symbol inserted function. It'll think about
			 * building the tree and populating it
			 */
			on_symbol_inserted (dbe, curr_symbol_id, (gpointer)dbvl);
		} while (symbol_db_engine_iterator_move_next (iterator) == TRUE);
		
		g_object_unref (iterator);
	}

	DEBUG_PRINT ("symbol_db_view_locals_update_list (): waiting for displaying: %d", 
				 g_tree_nnodes (priv->waiting_for));
	DEBUG_PRINT ("symbol_db_view_locals_update_list (): already displayed: %d", 
				 g_tree_nnodes (priv->nodes_displayed));

	/* ok, there may be some symbols left on the waiting_for_list...
 	 * launch the callback function by hand, flushing the list it in case 
	 */
	on_scan_end (dbe, dbvl);

	
	/* only gtk 2.12 ...
	 * gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (dbvl)); */
	
	gtk_tree_view_expand_all (GTK_TREE_VIEW (dbvl));
	
	/* set here the reference just to permit signal disconnecting */
	priv->dbe = dbe;
	
	/* connect some signals */
	if (priv->insert_handler <= 0) 
	{
		priv->insert_handler = 	g_signal_connect (G_OBJECT (dbe), "symbol_inserted",
					  G_CALLBACK (on_symbol_inserted), dbvl);
	}

	if (priv->remove_handler <= 0)
	{
		priv->remove_handler = g_signal_connect (G_OBJECT (dbe), "symbol_removed",
					  G_CALLBACK (on_symbol_removed), dbvl);
	}	

	if (priv->scan_end_handler <= 0)
	{
		priv->remove_handler = g_signal_connect (G_OBJECT (dbe), "scan_end",
					  G_CALLBACK (on_scan_end), dbvl);
	}	
}

 
