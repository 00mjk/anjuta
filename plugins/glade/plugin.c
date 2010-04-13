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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <config.h>

#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-file-savable.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <libanjuta/interfaces/ianjuta-help.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>

#include "plugin.h"
#include "anjuta-design-document.h"

#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-glade.ui"

static gpointer parent_class;

/* This is variable required from libdevhelp */
gchar *geometry = NULL;

struct _GladePluginPriv
{
	gint uiid;
	GtkActionGroup *action_group;
	GladeApp  *gpw;
	GtkWidget *inspector;
	GtkWidget *view_box;
	GtkWidget *projects_combo;
	gint editor_watch_id;
	gboolean destroying;
};

enum {
	NAME_COL,
	PROJECT_COL,
	N_COLUMNS
};

static void 
update_current_project (GtkComboBox *projects_combo,
                        GladeProject* project)
{	
	GtkTreeIter iter;
	GladeProject *cur_project;	
	GtkTreeModel* model = gtk_combo_box_get_model (projects_combo);
	
	if (gtk_tree_model_get_iter_first (model, &iter))
		do
		{
			gtk_tree_model_get (model, &iter, PROJECT_COL, &cur_project, -1);
			if (project == cur_project)
			{
				gtk_combo_box_set_active_iter (projects_combo, &iter);
				break;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	//GladePlugin* glade_plugin = ANJUTA_PLUGIN_GLADE(plugin);
	GObject *editor;	
	editor = g_value_get_object (value);
	if (ANJUTA_IS_DESIGN_DOCUMENT(editor))
	{
		AnjutaDesignDocument* view = ANJUTA_DESIGN_DOCUMENT(editor);
		GladeProject* project = glade_design_view_get_project(GLADE_DESIGN_VIEW(view));
		if (!view->is_project_added)
		{
			glade_app_add_project (project);
			view->is_project_added = TRUE;
		}
		glade_app_set_project (project);
	}
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	
}

static void
glade_update_ui (GladeApp *app, GladePlugin *plugin)
{
	IAnjutaDocument* doc;
	IAnjutaDocumentManager* docman = 
		anjuta_shell_get_interface(ANJUTA_PLUGIN(plugin)->shell,
								   IAnjutaDocumentManager, NULL);
	
	update_current_project (GTK_COMBO_BOX (plugin->priv->projects_combo), glade_app_get_project ());
	/* Emit IAnjutaDocument signal */	
	doc = ianjuta_document_manager_get_current_document(docman, NULL);
	if (doc && ANJUTA_IS_DESIGN_DOCUMENT(doc))
	{
		gboolean dirty = ianjuta_file_savable_is_dirty(IANJUTA_FILE_SAVABLE(doc), NULL);
		g_signal_emit_by_name (G_OBJECT(doc), "update_ui");
		g_signal_emit_by_name (G_OBJECT(doc), "save_point", !dirty);
	}
	
}

static void
on_api_help (GladeEditor* editor, 
			 const gchar* book,
			 const gchar* page,
			 const gchar* search,
			 GladePlugin* plugin)
{
	gchar *book_comm = NULL, *page_comm = NULL;
	gchar *string;

	AnjutaPlugin* aplugin = ANJUTA_PLUGIN(plugin);
	AnjutaShell* shell = aplugin->shell;
	IAnjutaHelp* help;
	
	help = anjuta_shell_get_interface(shell, IAnjutaHelp, NULL);
	
	/* No API Help Plugin */
	if (help == NULL)
		return;
	
	if (book) book_comm = g_strdup_printf ("book:%s ", book);
	if (page) page_comm = g_strdup_printf ("page:%s ", page);

	string = g_strdup_printf ("%s%s%s",
		book_comm ? book_comm : "",
		page_comm ? page_comm : "",
		search ? search : "");

	ianjuta_help_search(help, string, NULL);

	g_free (string);
}

static void
glade_do_close (GladePlugin *plugin, GladeProject *project)
{
	glade_app_remove_project (project);
}

static void
on_document_destroy (GtkWidget* document, GladePlugin *plugin)
{
	GladeProject *project;
	GtkTreeModel *model;
	GtkTreeIter iter;

	DEBUG_PRINT ("%s", "Destroying Document");
	
	if (plugin->priv->destroying)
	{
		return;
	}
	
	project = glade_design_view_get_project(GLADE_DESIGN_VIEW(document));

	if (!project)
	{
		return;
	}
		
	/* Remove project from our list */
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (plugin->priv->projects_combo));
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			GladeProject *project_node;
			
			gtk_tree_model_get (model, &iter, PROJECT_COL, &project_node, -1);
			if (project == project_node)
			{
				gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
				break;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}
	glade_do_close (plugin, project);

	if (gtk_tree_model_iter_n_children (model, NULL) <= 0)
		anjuta_plugin_deactivate (ANJUTA_PLUGIN (plugin));
}

static void
on_shell_destroy (AnjutaShell* shell, GladePlugin *glade_plugin)
{
	glade_plugin->priv->destroying = TRUE;
}

static void
on_glade_project_changed (GtkComboBox *combo, GladePlugin *plugin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	IAnjutaDocumentManager* docman = 
		anjuta_shell_get_interface(ANJUTA_PLUGIN(plugin)->shell,
								   IAnjutaDocumentManager, NULL);
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (plugin->priv->projects_combo));
	if (gtk_combo_box_get_active_iter (combo, &iter))
	{
		GladeProject *project;
		
		GtkWidget *design_view;
		gtk_tree_model_get (model, &iter, PROJECT_COL, &project, -1);
		glade_app_set_project (project);
		
		design_view = g_object_get_data (G_OBJECT (project), "design_view");
		ianjuta_document_manager_set_current_document(docman, IANJUTA_DOCUMENT(design_view), NULL);
		
#  if (GLADEUI_VERSION >= 330)
        glade_inspector_set_project (GLADE_INSPECTOR (plugin->priv->inspector), project);
#  endif								   
									   
	}
}

static void
register_stock_icons (AnjutaPlugin *plugin)
{
	static gboolean registered = FALSE;

	if (registered)
		return;
	registered = TRUE;

	/* Register stock icons */
	BEGIN_REGISTER_ICON (plugin);
	REGISTER_ICON ("anjuta-glade-plugin-48.png",
				   "glade-plugin-icon");
	END_REGISTER_ICON;
}

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase,
				 AnjutaSession *session, GladePlugin *plugin)
{
	GList *files, *docwids, *node;
/*	GtkTreeModel *model;
	GtkTreeIter iter;
*/
	IAnjutaDocumentManager *docman;
	
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;
	
	docman = anjuta_shell_get_interface (ANJUTA_PLUGIN(plugin)->shell,
										 IAnjutaDocumentManager, NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		files = anjuta_session_get_string_list (session, "File Loader", "Files");
		if (files)
			files = g_list_reverse (files);
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				GFile* file;
				file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				if (file != NULL)
				{
					files = g_list_prepend (files, g_file_get_uri (file));
					/* uri is not freed here */
				}
				g_object_unref (file);
			}
		}
		g_list_free (docwids);
		if (files)
		{
			files = g_list_reverse (files);
			anjuta_session_set_string_list (session, "File Loader", "Files", files);
			g_list_foreach (files, (GFunc)g_free, NULL);
			g_list_free (files);
		}
	}
/*
	files = anjuta_session_get_string_list (session, "File Loader", "Files");
	if (files)
		files = g_list_reverse (files);
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (plugin->priv->projects_combo));
	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
#if (GLADEUI_VERSION >= 330)
			const gchar *ppath;
#endif
			gchar *uri;
			GladeProject *project;
			gtk_tree_model_get (model, &iter, PROJECT_COL, &project, -1);
#if (GLADEUI_VERSION >= 330)
			ppath = glade_project_get_path (project);
			if (ppath)
#else
			if (project->path)
#endif
			{
				GFile *file = g_file_new_for_path (
#if (GLADEUI_VERSION >= 330)
					ppath);
#else
					project->path);
#endif
				uri = g_file_get_uri (file);
				g_object_unref (file);
				if (uri)
				{
					/ * FIXME only log file if it's still open in docman * /
					files = g_list_prepend (files, uri);
					/ * uri is not freed here * /
				}
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}

	if (files)
	{
		files = g_list_reverse (files);
		anjuta_session_set_string_list (session, "File Loader", "Files", files);
		g_list_foreach (files, (GFunc)g_free, NULL);
		g_list_free (files);
	}
*/
}

static void
glade_plugin_add_project (GladePlugin *glade_plugin, GladeProject *project)
{
	GtkWidget *view;
	GladePluginPriv *priv;
	IAnjutaDocumentManager* docman = 
		anjuta_shell_get_interface(ANJUTA_PLUGIN(glade_plugin)->shell,
								   IAnjutaDocumentManager, NULL);
	
 	g_return_if_fail (GLADE_IS_PROJECT (project));
 	
	priv = glade_plugin->priv;
 	view = anjuta_design_document_new(glade_plugin, project);
	g_signal_connect (G_OBJECT(view), "destroy",
					  G_CALLBACK (on_document_destroy), glade_plugin);
	gtk_widget_show (view);
	g_object_set_data (G_OBJECT (project), "design_view", view);
	ianjuta_document_manager_add_document(docman, IANJUTA_DOCUMENT(view), NULL);
}

static void
inspector_item_activated_cb (GladeInspector     *inspector,
							 AnjutaPlugin       *plugin)
{
	GList *item = glade_inspector_get_selected_items (inspector);
	g_assert (GLADE_IS_WIDGET (item->data) && (item->next == NULL));
	
	/* switch to this widget in the workspace */
	glade_widget_show (GLADE_WIDGET (item->data));
	
	g_list_free (item);
}

static void
on_glade_resource_removed (GladeProject *project, GladePlugin *plugin)
{
}

static void
on_glade_resource_updated (GladeProject *project, GladePlugin *plugin)
{
}

static gboolean
activate_plugin (AnjutaPlugin *plugin)
{
	AnjutaUI *ui;
	GladePlugin *glade_plugin;
	GladePluginPriv *priv;
	// GtkAction *action;
	GtkListStore *store;
	GtkCellRenderer *renderer;
	
	DEBUG_PRINT ("%s", "GladePlugin: Activating Glade plugin...");
	
	glade_plugin = ANJUTA_PLUGIN_GLADE (plugin);
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	priv = glade_plugin->priv;
	
	register_stock_icons (plugin);
	
	if (!priv->gpw)
	{
		priv->gpw = g_object_new(GLADE_TYPE_APP, NULL);
	
		glade_app_set_window (GTK_WIDGET (ANJUTA_PLUGIN(plugin)->shell));
		glade_app_set_transient_parent (GTK_WINDOW (ANJUTA_PLUGIN(plugin)->shell));
    	
		/* Create a view for us */
		priv->view_box = gtk_vbox_new (FALSE, 0);
		store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING,
									G_TYPE_POINTER, NULL);
		
		priv->projects_combo = gtk_combo_box_new ();
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->projects_combo),
									renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->projects_combo),
										renderer, "text", NAME_COL, NULL);
		gtk_combo_box_set_model (GTK_COMBO_BOX (priv->projects_combo),
								 GTK_TREE_MODEL (store));
		g_object_unref (G_OBJECT (store));
		gtk_box_pack_start (GTK_BOX (priv->view_box), priv->projects_combo,
							FALSE, FALSE, 0);

        priv->inspector = glade_inspector_new ();
        
        g_signal_connect (priv->inspector, "item-activated",
        				  G_CALLBACK (inspector_item_activated_cb),
        				  plugin);		

		gtk_box_pack_start (GTK_BOX (priv->view_box), GTK_WIDGET (priv->inspector),
							TRUE, TRUE, 0);
		
		gtk_widget_show_all (priv->view_box);
		gtk_notebook_set_scrollable (GTK_NOTEBOOK (glade_app_get_editor ()->notebook),
									 TRUE);
		gtk_notebook_popup_enable (GTK_NOTEBOOK (glade_app_get_editor ()->notebook));
	}
	
	g_signal_connect(G_OBJECT(plugin->shell), "destroy",
					 G_CALLBACK(on_shell_destroy), plugin);
	
	g_signal_connect (G_OBJECT (priv->projects_combo), "changed",
					  G_CALLBACK (on_glade_project_changed), plugin);
	g_signal_connect (G_OBJECT (priv->gpw), "update-ui",
					  G_CALLBACK (glade_update_ui), plugin);
	
	g_signal_connect(G_OBJECT(glade_app_get_editor()), "gtk-doc-search",
					 G_CALLBACK(on_api_help), plugin);
	
	/* FIXME: Glade doesn't want to die these widget, so
	 * hold a permenent refs on them
	 */
	g_object_ref (glade_app_get_palette ());
	g_object_ref (glade_app_get_editor ());
	g_object_ref (priv->view_box);

	gtk_widget_show (GTK_WIDGET (glade_app_get_palette ()));
	gtk_widget_show (GTK_WIDGET (glade_app_get_editor ()));
	
	/* Add widgets */
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (priv->view_box),
							 "AnjutaGladeTree", _("Widgets"),
							 "glade-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (glade_app_get_palette ()),
							 "AnjutaGladePalette", _("Palette"),
							 "glade-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_LEFT, NULL);
	anjuta_shell_add_widget (ANJUTA_PLUGIN (plugin)->shell,
							 GTK_WIDGET (glade_app_get_editor ()),
							 "AnjutaGladeEditor", _("Properties"),
							 "glade-plugin-icon",
							 ANJUTA_SHELL_PLACEMENT_CENTER, NULL);
	/* Connect to save session */
	g_signal_connect (G_OBJECT (plugin->shell), "save_session",
					  G_CALLBACK (on_session_save), plugin);
	
	/* Watch documents */
	glade_plugin->priv->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, IANJUTA_DOCUMENT_MANAGER_CURRENT_DOCUMENT,
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	
	return TRUE;
}

static gboolean
deactivate_plugin (AnjutaPlugin *plugin)
{
	GladePluginPriv *priv;
	
	priv = ANJUTA_PLUGIN_GLADE (plugin)->priv;
		
	DEBUG_PRINT ("%s", "GladePlugin: Dectivating Glade plugin...");
	
	/* Disconnect signals */
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_shell_destroy),
										  plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->shell),
										  G_CALLBACK (on_session_save), plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (priv->projects_combo),
										  G_CALLBACK (on_glade_project_changed),
										  plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (priv->gpw),
										  G_CALLBACK (glade_update_ui),
										  plugin);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT(glade_app_get_editor()),
										  G_CALLBACK(on_api_help), plugin);

	/* Remove widgets */
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (glade_app_get_palette ()),
								NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (glade_app_get_editor ()),
								NULL);
	anjuta_shell_remove_widget (plugin->shell,
								GTK_WIDGET (priv->view_box),
								NULL);
	/* FIXME: Don't destroy glade, since it doesn't want to */
	/* g_object_unref (G_OBJECT (priv->gpw)); */
	/* priv->gpw = NULL */
	
	priv->uiid = 0;
	priv->action_group = NULL;
	priv->gpw = NULL;
	
	return TRUE;
}

static void
glade_plugin_dispose (GObject *obj)
{
	/* GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj); */
	
	/* FIXME: Glade widgets should be destroyed */
	G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
glade_plugin_finalize (GObject *obj)
{
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj);
	g_free (plugin->priv);
	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
glade_plugin_instance_init (GObject *obj)
{
	GladePluginPriv *priv;
	GladePlugin *plugin = ANJUTA_PLUGIN_GLADE (obj);
	
	plugin->priv = (GladePluginPriv *) g_new0 (GladePluginPriv, 1);
	priv = plugin->priv;
	priv->destroying = FALSE;
	
	DEBUG_PRINT ("%s", "Intializing Glade plugin");
}

static void
glade_plugin_class_init (GObjectClass *klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = glade_plugin_dispose;
	klass->finalize = glade_plugin_finalize;
}

gchar* glade_get_filename(GladePlugin *plugin)
{
	return glade_project_get_name(glade_app_get_project());
}

static void
ifile_open (IAnjutaFile *ifile, GFile* file, GError **err)
{
	GladePluginPriv *priv;
	GladeProject *project;
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *filename;
	IAnjutaDocumentManager* docman;
	GList* docwids, *node;
	GList *glade_obj_node;
	
	g_return_if_fail (file != NULL);
	
	priv = ANJUTA_PLUGIN_GLADE (ifile)->priv;
	
	filename = g_file_get_path (file);
	if (!filename)
	{
		gchar* uri = g_file_get_parse_name(file);
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
								    _("Not local file: %s"), uri);
		g_free (uri);
		return;
	}
	
	docman = anjuta_shell_get_interface(ANJUTA_PLUGIN(ifile)->shell, IAnjutaDocumentManager,
										NULL);
	docwids = ianjuta_document_manager_get_doc_widgets (docman, NULL);
	if (docwids)
	{
		for (node = docwids; node != NULL; node = g_list_next (node))
		{
			if (ANJUTA_IS_DESIGN_DOCUMENT (node->data))
			{
				GFile* cur_file;
				cur_file = ianjuta_file_get_file (IANJUTA_FILE (node->data), NULL);
				if (cur_file)
				{
					if (g_file_equal (file, cur_file))
					{
						ianjuta_document_manager_set_current_document (docman,
							IANJUTA_DOCUMENT (node->data), NULL);
						g_object_unref (file);
						return;
					}
					g_object_unref (file);
				}
			}
		}
		g_list_free (docwids);
	}
	
	project = glade_project_load (filename);
	if (project)
	{
		g_signal_connect (project, "resource-removed",
						  G_CALLBACK (on_glade_resource_removed),
						  ANJUTA_PLUGIN_GLADE (ifile));
		g_signal_connect (project, "resource-updated",
						  G_CALLBACK (on_glade_resource_updated),
						  ANJUTA_PLUGIN_GLADE (ifile));
	}
	g_free (filename);
	if (!project)
	{
		gchar* name = g_file_get_parse_name (file);
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (ifile)->shell),
								    _("Could not open %s"), name);
		g_free (name);
		return;
	}
	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->projects_combo)));
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, NAME_COL, glade_project_get_name(project),
						PROJECT_COL, project, -1);
	glade_plugin_add_project (ANJUTA_PLUGIN_GLADE (ifile), project);

	/* Select the first window in the project */
	for (glade_obj_node = (GList *) glade_project_get_objects (project);
		 glade_obj_node != NULL;
		 glade_obj_node = g_list_next (glade_obj_node))
	{
		GObject *glade_obj = G_OBJECT (glade_obj_node->data);
		if (GTK_IS_WINDOW (glade_obj))
			glade_widget_show (glade_widget_get_from_gobject (glade_obj));
		break;
	}
	anjuta_shell_present_widget (ANJUTA_PLUGIN (ifile)->shell, priv->view_box, NULL);
}

static GFile*
ifile_get_file (IAnjutaFile* ifile, GError** e)
{
	const gchar* path = glade_project_get_path(glade_app_get_project());
	GFile* file = g_file_new_for_path (path);
	return file;
}

static void
ifile_iface_init(IAnjutaFileIface *iface)
{
	iface->open = ifile_open;
	iface->get_file = ifile_get_file;
}

static void
iwizard_activate (IAnjutaWizard *iwizard, GError **err)
{
	GladePluginPriv *priv;
	GladeProject *project;
	GtkListStore *store;
	GtkTreeIter iter;
	
	priv = ANJUTA_PLUGIN_GLADE (iwizard)->priv;

	project = glade_project_new ();
	
	g_signal_connect (project, "resource-removed",
					  G_CALLBACK (on_glade_resource_removed),
					  ANJUTA_PLUGIN_GLADE (iwizard));
	g_signal_connect (project, "resource-updated",
					  G_CALLBACK (on_glade_resource_updated),
					  ANJUTA_PLUGIN_GLADE (iwizard));
	if (!project)
	{
		anjuta_util_dialog_warning (GTK_WINDOW (ANJUTA_PLUGIN (iwizard)->shell),
								    _("Could not create a new glade project."));
		return;
	}
	store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (priv->projects_combo)));
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, NAME_COL, glade_project_get_name(project),
 						PROJECT_COL, project, -1);
	glade_plugin_add_project (ANJUTA_PLUGIN_GLADE (iwizard), project);
	anjuta_shell_present_widget (ANJUTA_PLUGIN (iwizard)->shell,
				     GTK_WIDGET (glade_app_get_palette ()), NULL);
}

static void
iwizard_iface_init(IAnjutaWizardIface *iface)
{
	iface->activate = iwizard_activate;
}

ANJUTA_PLUGIN_BEGIN (GladePlugin, glade_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE (ifile, IANJUTA_TYPE_FILE);
ANJUTA_PLUGIN_ADD_INTERFACE (iwizard, IANJUTA_TYPE_WIZARD);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GladePlugin, glade_plugin);
