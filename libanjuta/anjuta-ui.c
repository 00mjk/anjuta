/*
    anjuta-ui.c
    Copyright (C) 2003  Naba Kumar  <naba@gnome.org>

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

#include <stdio.h>
#include <string.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkaccelmap.h>
#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkimage.h>

#include <libgnome/libgnome.h>
#include <libegg/treeviewutils/eggcellrendererkeys.h>

#include "pixmaps.h"
#include "resources.h"
#include "anjuta-ui.h"

struct _AnjutaUIPrivate {
	GtkUIManager *merge;
	GtkIconFactory *icon_factory;
	GtkTreeModel *model;
	GHashTable *actions_hash;
};

enum {
	COLUMN_PIXBUF,
	COLUMN_ACTION,
	COLUMN_VISIBLE,
	COLUMN_SENSITIVE,
	COLUMN_DATA,
	COLUMN_GROUP,
	N_COLUMNS
};

#if 0
static gboolean
on_delete_event (AnjutaUI *ui, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (ui));
	return FALSE;
}
#endif

static void
sensitivity_toggled (GtkCellRendererToggle *cell,
					 const gchar *path_str, GtkTreeModel *model)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkAction *action;
	gboolean sensitive;
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	
	gtk_tree_model_get (model, &iter,
						COLUMN_SENSITIVE, &sensitive,
						COLUMN_DATA, &action, -1);
	g_object_set (G_OBJECT (action), "sensitive", !sensitive, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						COLUMN_SENSITIVE, !sensitive, -1);
	gtk_tree_path_free (path);
}

static void
visibility_toggled (GtkCellRendererToggle *cell,
					const gchar *path_str, GtkTreeModel *model)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkAction *action;
	gboolean visible;
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	
	gtk_tree_model_get (model, &iter,
						COLUMN_VISIBLE, &visible,
						COLUMN_DATA, &action, -1);
	g_object_set (G_OBJECT (action), "visible", !visible, NULL);
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
						COLUMN_VISIBLE, !visible, -1);
	gtk_tree_path_free (path);
}

static gchar*
get_action_label (GtkAction *action)
{
	gchar *action_label;
/* FIXME: we need to get the label of the action */
#if 0
	if (action->label && strlen (action->label))
	{
		gchar *s, *d;
		action_label = g_strdup (action->label);
		
		s = d = action_label;
		while (*s)
		{
			/* FIXME: May break with multibyte chars */
			if (*s == '_')
				s++;
			*d = *s; d++; s++;
		}
		*d = '\0';
	}
	else
#endif
		action_label = g_strdup (gtk_action_get_name (action));
	return action_label;
}

static gchar*
get_action_accel_path (GtkAction *action, const gchar *group_name)
{
	gchar *accel_path;
	accel_path = g_strconcat ("<Actions>/", group_name,
							  "/", gtk_action_get_name (action), NULL);
	return accel_path;
}

static gchar*
get_action_accel (GtkAction *action, const gchar *group_name)
{
	gchar *accel_path;
	gchar *accel_name;
	GtkAccelKey key;
	
	accel_path = get_action_accel_path (action, group_name);
	if ( gtk_accel_map_lookup_entry (accel_path, &key))
		accel_name = gtk_accelerator_name (key.accel_key, key.accel_mods);
	else
		accel_name = strdup ("");
	g_free (accel_path);
	return accel_name;
}

static void
accel_edited_callback (GtkCellRendererText *cell,
                       const char          *path_string,
                       guint                keyval,
                       GdkModifierType      mask,
                       guint                hardware_keycode,
                       gpointer             data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	GtkAction *action;
	gchar *accel_path, *action_group;
	
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
						COLUMN_DATA, &action,
						COLUMN_GROUP, &action_group, -1);
	
	/* sanity check */
	if (action == NULL || action_group == NULL)
		return;
	
	accel_path = get_action_accel_path (action, action_group);
	if (accel_path) {
		gtk_accel_map_change_entry (accel_path, keyval, mask, TRUE);
		g_free (accel_path);
	}
	
	gtk_tree_path_free (path);
}

static gint
iter_compare_func (GtkTreeModel *model, GtkTreeIter *a,
				   GtkTreeIter *b, gpointer user_data)
{
	const gchar *text_a;
	const gchar *text_b;
	gint retval = 0;
	
	gtk_tree_model_get (model, a, COLUMN_ACTION, &text_a, -1);
	gtk_tree_model_get (model, b, COLUMN_ACTION, &text_b, -1);
	if (text_a == NULL && text_b == NULL) retval = 0;
	else if (text_a == NULL) retval = -1;
	else if (text_b == NULL) retval = 1;
	else retval =  strcasecmp (text_a, text_b);
	return retval;
}

static gboolean
binding_from_string (const char      *str,
                     guint           *accelerator_key,
                     GdkModifierType *accelerator_mods)
{
	EggVirtualModifierType virtual;
	
	g_return_val_if_fail (accelerator_key != NULL, FALSE);
	
	if (str == NULL || (str && strcmp (str, "disabled") == 0))
	{
		*accelerator_key = 0;
		*accelerator_mods = 0;
		return TRUE;
	}
	
	if (!egg_accelerator_parse_virtual (str, accelerator_key, &virtual))
		return FALSE;
	
	egg_keymap_resolve_virtual_modifiers (gdk_keymap_get_default (),
										  virtual,
										  accelerator_mods);
	
	/* Be sure the GTK accelerator system will be able to handle this
	* accelerator. Be sure to allow no-accelerator accels like F1.
	*/
	if ((*accelerator_mods & gtk_accelerator_get_default_mod_mask ()) == 0 &&
		*accelerator_mods != 0)
		return FALSE;
	
	if (*accelerator_key == 0)
		return FALSE;
	else
		return TRUE;
}

static void
accel_set_func (GtkTreeViewColumn *tree_column,
                GtkCellRenderer   *cell,
                GtkTreeModel      *model,
                GtkTreeIter       *iter,
                gpointer           data)
{
	GtkAction *action;
	gchar *accel_name;
	gchar *group_name;
	guint keyval;
	GdkModifierType keymods;
	
	gtk_tree_model_get (model, iter,
						COLUMN_DATA, &action,
						COLUMN_GROUP, &group_name, -1);
	if (action == NULL)
		g_object_set (G_OBJECT (cell), "visible", FALSE, NULL);
	else
	{
		accel_name = get_action_accel (action, group_name);
		if (binding_from_string (accel_name, &keyval, &keymods))
		{
			g_object_set (G_OBJECT (cell), "visible", TRUE,
						  "accel_key", keyval,
						  "accel_mask", keymods, NULL);
		}
		else
			g_object_set (G_OBJECT (cell), "visible", TRUE,
						  "accel_key", 0,
						  "accel_mask", 0, NULL);
		g_free (accel_name);	
	}
}

static GtkWidget *
create_tree_view (AnjutaUI *ui)
{
	GtkWidget *tree_view, *sw;
	GtkTreeStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	
	store = gtk_tree_store_new (N_COLUMNS,
								GDK_TYPE_PIXBUF,
								G_TYPE_STRING,
								G_TYPE_BOOLEAN,
								G_TYPE_BOOLEAN,
								G_TYPE_POINTER,
								G_TYPE_STRING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(store), COLUMN_ACTION,
									 iter_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(store),
										  COLUMN_ACTION, GTK_SORT_ASCENDING);
	
	tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_view), TRUE);
	
	/* Columns */
	column = gtk_tree_view_column_new ();
	// gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_title (column, _("Action"));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer, "pixbuf",
										COLUMN_PIXBUF);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer, "text",
										COLUMN_ACTION);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	gtk_tree_view_set_expander_column (GTK_TREE_VIEW (tree_view), column);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (visibility_toggled), store);
	column = gtk_tree_view_column_new_with_attributes (_("Visible"),
													   renderer,
													   "active",
													   COLUMN_VISIBLE,
													   NULL);
	// gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (renderer), "toggled",
					  G_CALLBACK (sensitivity_toggled), store);
	column = gtk_tree_view_column_new_with_attributes (_("Sensitive"),
													   renderer,
													   "active",
													   COLUMN_SENSITIVE,
													   NULL);
	// gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Shortcut"));
	// gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);	
	renderer = g_object_new (EGG_TYPE_CELL_RENDERER_KEYS,
							"editable", TRUE,
							"accel_mode", EGG_CELL_RENDERER_KEYS_MODE_GTK,
							NULL);
	g_signal_connect (G_OBJECT (renderer), "keys_edited",
					  G_CALLBACK (accel_edited_callback),
					  store);
	g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, renderer, accel_set_func, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATA);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
									GTK_POLICY_AUTOMATIC,
									GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (sw), tree_view);
	
	// unreferenced in destroy() method.
	ui->priv->model = GTK_TREE_MODEL (store);
	
	return sw;
}

static void anjuta_ui_class_init (AnjutaUIClass *class);
static void anjuta_ui_instance_init (AnjutaUI *ui);

GNOME_CLASS_BOILERPLATE (AnjutaUI, 
						 anjuta_ui,
						 GtkDialog, GTK_TYPE_DIALOG);

static void
anjuta_ui_dispose (GObject *obj)
{
	AnjutaUI *ui = ANJUTA_UI (obj);

	if (ui->priv->model) {
		g_object_unref (G_OBJECT (ui->priv->model));
		ui->priv->model = NULL;
	}
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}

static void
anjuta_ui_finalize (GObject *obj)
{
	AnjutaUI *ui = ANJUTA_UI (obj);	
	g_free (ui->priv);
	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (obj));
}

static void
anjuta_ui_close (GtkDialog *dialog)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
anjuta_ui_class_init (AnjutaUIClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (class);

	parent_class = g_type_class_peek_parent (class);
	
	object_class->dispose = anjuta_ui_dispose;
	object_class->finalize = anjuta_ui_finalize;

	// dialog_class->response = anjuta_preferences_dialog_response;
	dialog_class->close = anjuta_ui_close;
}

static void
anjuta_ui_instance_init (AnjutaUI *ui)
{
	GtkWidget *view;
	
	ui->priv = g_new0 (AnjutaUIPrivate, 1);
	ui->priv->merge = gtk_ui_manager_new();
	ui->priv->actions_hash = g_hash_table_new_full (g_str_hash,
													g_str_equal,
													(GDestroyNotify) g_free,
													(GDestroyNotify) NULL);
	ui->priv->icon_factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (ui->priv->icon_factory);
	
	gtk_window_set_default_size (GTK_WINDOW (ui), 500, 400);
	view = create_tree_view (ui);
	gtk_widget_show_all (view);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(ui)->vbox), view);
}

/**
 * anjuta_ui_new:
 * @ui_container: The container where UI objects (menus and toolbars) will
 * be added.
 * @add_widget_cb: Callback function to call for adding the widget in
 * ui_container.
 * @remove_widget_cb: Callback function to call for removing the widget from
 * ui_container.
 * 
 * Creates a new instance of #AnjutaUI. @ui_container is the container where
 * where UI elements (menus and toolbars) will be added using the callback
 * @add_widget_cb. @remove_widget_cb will be called for removing the UI
 * elements from @ui_container. These two callbacks are directly connected to
 * the "add_widget" and "remove_widget" signals of GtkUIManager class, so the
 * the prototypes for these callbacks should be based on them.
 * 
 * Return value: A #AnjutaUI object
 */
GtkWidget *
anjuta_ui_new (GtkWidget *ui_container,
			   GCallback add_widget_cb, GCallback remove_widget_cb)
{
	GtkWidget *widget;
	widget = gtk_widget_new (ANJUTA_TYPE_UI, 
				 "title", _("Anjuta User Interface"),
				 NULL);
	if (add_widget_cb)
		g_signal_connect (G_OBJECT (ANJUTA_UI (widget)->priv->merge),
						  "add_widget", G_CALLBACK (add_widget_cb),
	/* FIXME: */					  ui_container);
	/* if (remove_widget_cb)
		g_signal_connect (G_OBJECT (ANJUTA_UI (widget)->priv->merge),
						  "remove_widget", G_CALLBACK (remove_widget_cb),
					  ui_container);*/
	return widget;
}

/**
 * anjuta_ui_get_icon_factory:
 * @ui: A #AnjutaUI object
 * 
 * This returns the IconFactory object. All icons should be registered using
 * this icon factory. Read the documentation for #GtkIconFactory on how to 
 * use it.
 *
 * Return value: The #GtkIconFactory object used by it
 */
GtkIconFactory*
anjuta_ui_get_icon_factory (AnjutaUI *ui)
{
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	return ui->priv->icon_factory;
}

/**
 * anjuta_ui_add_action_group_entries:
 * @ui: A #AnjutaUI object.
 * @action_group_name: Untranslated name of the action group.
 * @action_group_label: Translated label of the action group.
 * @entries: An array of action entries.
 * @num_entries: Number of elements in the action entries array.
 * @user_data: User data to pass to action objects. This is the data that
 * will come as user_data in "activate" signal of the actions.
 * 
 * #GtkAction objects are created from the #GtkActionEntry structures and
 * added to the UI Manager. "activate" signal of #GtkAction is connected for
 * all the action objects using the callback in the entry structure and the
 * @user_data passed here.
 *
 * This group of actions are registered with the name @action_group_name
 * in #AnjutaUI. A #GtkAction object from this action group can be later
 * retrieved by anjuta_ui_get_action() using @action_group_name and action name.
 * @action_group_label is used as the display name for the action group in
 * UI manager dialog where action shortcuts are configured.
 * 
 * Return value: A #GtkActionGroup object holding all the action objects.
 */
GtkActionGroup*
anjuta_ui_add_action_group_entries (AnjutaUI *ui,
									const gchar *action_group_name,
									const gchar *action_group_label,
									GtkActionEntry *entries,
									gint num_entries,
									gpointer user_data)
{
	GtkActionGroup *action_group;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	
	action_group = gtk_action_group_new (action_group_name);
	gtk_action_group_add_actions (action_group, entries, num_entries,
								  user_data);
	anjuta_ui_add_action_group (ui, action_group_name,
								action_group_label, action_group);
	// g_object_unref (action_group);
	return action_group;
}

/**
 * anjuta_ui_add_toggle_action_group_entries:
 * @ui: A #AnjutaUI object.
 * @action_group_name: Untranslated name of the action group.
 * @action_group_label: Translated label of the action group.
 * @entries: An array of action entries.
 * @num_entries: Number of elements in the action entries array.
 * @user_data: User data to pass to action objects. This is the data that
 * will come as user_data in "activate" signal of the actions.
 * 
 * This is similar to anjuta_ui_add_action_group_entries(), except that
 * it adds #GtkToggleAction objects after creating them from the @entries.
 * 
 * Return value: A #GtkActionGroup object holding all the action objects.
 */
GtkActionGroup*
anjuta_ui_add_toggle_action_group_entries (AnjutaUI *ui,
									const gchar *action_group_name,
									const gchar *action_group_label,
									GtkToggleActionEntry *entries,
									gint num_entries,
									gpointer user_data)
{
	GtkActionGroup *action_group;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	g_return_val_if_fail (action_group_name != NULL, NULL);
	
	action_group = gtk_action_group_new (action_group_name);
	gtk_action_group_add_toggle_actions (action_group, entries, num_entries,
										 user_data);
	anjuta_ui_add_action_group (ui, action_group_name,
								action_group_label, action_group);
	// g_object_unref (action_group);
	return action_group;
}

/**
 * anjuta_ui_add_action_group:
 * @ui: A #AnjutaUI object.
 * @action_group_name: Untranslated name of the action group.
 * @action_group_label: Translated label of the action group.
 * @action_group: #GtkActionGroup object to add.
 * 
 * This is similar to anjuta_ui_add_action_group_entries(), except that
 * it adds #GtkActionGroup object @action_group directly. All actions in this
 * group are automatically registered in #AnjutaUI and can be retrieved
 * normally with anjuta_ui_get_action().
 */
void
anjuta_ui_add_action_group (AnjutaUI *ui,
							const gchar *action_group_name,
							const gchar *action_group_label,
							GtkActionGroup *action_group)
{
	GList *actions, *l;
	GtkTreeIter parent;
	GdkPixbuf *pixbuf;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (GTK_IS_ACTION_GROUP (action_group));
	g_return_if_fail (action_group_name != NULL);
	g_return_if_fail (action_group_name != NULL);
	
	gtk_ui_manager_insert_action_group (ui->priv->merge, action_group, 0);
	g_hash_table_insert (ui->priv->actions_hash,
						g_strdup (action_group_name), action_group);
	actions = gtk_action_group_list_actions (action_group);
	gtk_tree_store_append (GTK_TREE_STORE (ui->priv->model),
						   &parent, NULL);
	pixbuf = anjuta_res_get_pixbuf (ANJUTA_PIXMAP_CLOSED_FOLDER);
	gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &parent,
						COLUMN_PIXBUF, pixbuf,
						COLUMN_ACTION, action_group_label,
						COLUMN_GROUP, action_group_name,
						-1);
	for (l = actions; l; l = l->next)
	{
		gchar *action_label;
		// gchar *accel_name;
		GtkTreeIter iter;
		GtkWidget *icon;
		GtkAction *action = l->data;
		
		if (!action)
			continue;
		gtk_tree_store_append (GTK_TREE_STORE (ui->priv->model),
							   &iter, &parent);
		action_label = get_action_label (action);
		icon = gtk_action_create_icon (action, GTK_ICON_SIZE_MENU);
		if (icon)
		{
			pixbuf = NULL;
			// FIXME: Somehow get the pixbuf
			// pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (icon));
			gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &iter,
								COLUMN_PIXBUF, pixbuf,
								COLUMN_ACTION, action_label,
								COLUMN_VISIBLE, gtk_action_get_visible (action),
								COLUMN_SENSITIVE, gtk_action_get_sensitive(action),
								COLUMN_DATA, action,
								COLUMN_GROUP, action_group_name,
								-1);
			// g_object_unref (G_OBJECT (pixbuf));
			gtk_widget_destroy (icon);
		}
		else
		{
			gtk_tree_store_set (GTK_TREE_STORE (ui->priv->model), &iter,
								COLUMN_ACTION, action_label,
								COLUMN_VISIBLE, gtk_action_get_visible (action),
								COLUMN_SENSITIVE, gtk_action_get_sensitive (action),
								COLUMN_DATA, action,
								COLUMN_GROUP, action_group_name,
								-1);
		}
		g_free (action_label);
	}
}

static gboolean
on_action_group_remove_hash (gpointer key, gpointer value, gpointer data)
{
	if (data == value)
	{
#ifdef DEBUG
		g_message ("Removing action group from hash: %s",
				   gtk_action_group_get_name (GTK_ACTION_GROUP (data)));
#endif
		return TRUE;
	}
	else
		return FALSE;
}

/**
 * anjuta_ui_remove_action_group:
 * @ui: A #AnjutaUI object
 * @action_group: #GtkActionGroup object to remove.
 *
 * Removes a previous added action group. All actions in this group are
 * also unregistered from UI manager.
 */
void
anjuta_ui_remove_action_group (AnjutaUI *ui, GtkActionGroup *action_group)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;

	g_return_if_fail (ANJUTA_IS_UI (ui));
		
	const gchar *name;
	name = gtk_action_group_get_name (action_group);
	g_hash_table_foreach_remove (ui->priv->actions_hash,
								 on_action_group_remove_hash, action_group);
	
	model = ui->priv->model;
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		// GtkTreeIter parent;
		const gchar *group;
		const gchar *group_name;
		
		gtk_tree_model_get (model, &iter, COLUMN_GROUP, &group, -1);
		group_name = gtk_action_group_get_name (GTK_ACTION_GROUP (action_group));
		
		g_message ("%s == %s", group, group_name);
		if (group_name == NULL || group == NULL)
		{
			valid = gtk_tree_model_iter_next (model, &iter);
			continue;
		}
		if (strcmp (group_name, group) == 0)
		{
#ifdef DEBUG
			g_message ("Removing action group from tree: %s", group);
#endif
			valid = gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
		}
		else
			valid = gtk_tree_model_iter_next (model, &iter);
	}
	gtk_ui_manager_remove_action_group (ui->priv->merge, action_group);
}

/**
 * anjuta_ui_merge:
 * @ui: A #AnjutaUI object.
 * @ui_filename: UI file to merge into UI manager.
 *
 * Merges XML UI definition in @ui_filename. UI elements defined in the xml
 * are merged with existing UI elements in UI manager. The format of the
 * file content is the standard XML UI definition tree. For more detail,
 * read the documentation for #GtkUIManager.
 * 
 * Return value: Integer merge ID
 */
gint
anjuta_ui_merge (AnjutaUI *ui, const gchar *ui_filename)
{
	gint id;
	GError *err = NULL;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), -1);
	g_return_val_if_fail (ui_filename != NULL, -1);
	id = gtk_ui_manager_add_ui_from_file(ui->priv->merge, ui_filename, &err);
#ifdef DEBUG
	{
		gchar *basename = g_path_get_basename (ui_filename);
		g_message("merged [%d] %s", id, basename);
	}
#endif
	if (err != NULL)
		g_warning ("Could not merge [%s]: %s", ui_filename, err->message);
	return id;
}

/**
 * anjuta_ui_unmerge:
 * @ui: A #AnjutaUI object.
 * @id: Merge ID returned by anjuta_ui_merge().
 *
 * Unmerges UI with the ID value @id (returned by anjuta_ui_merge() when
 * it was merged. For more detail, read the documentation for #GtkUIManager.
 */
void
anjuta_ui_unmerge (AnjutaUI *ui, gint id)
{
#ifdef DEBUG
	g_message("Menu unmerging %d", id);
#endif
	g_return_if_fail (ANJUTA_IS_UI (ui));
	gtk_ui_manager_remove_ui(ui->priv->merge, id);
}

/**
 * anjuta_ui_get_accel_group:
 * @ui: A #AnjutaUI object.
 * returns: A #GtkAccelGroup object.
 *
 * Returns the #GtkAccelGroup object associated with this UI manager.
 */
GtkAccelGroup*
anjuta_ui_get_accel_group (AnjutaUI *ui)
{
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	return gtk_ui_manager_get_accel_group (ui->priv->merge);
}

/**
 * anjuta_ui_get_menu_merge:
 * @ui: A #AnjutaUI object.
 * returns: A #GtkUIManager object.
 *
 * Returns the #GtkUIManager object use by this UI manager. Please note that
 * any actions additions/removals using GtkUIManager are not registred with
 * #AnjutaUI and hence their accellerators cannot be edited. Nor will they be
 * listed in the UI manager dialog. Hence, use #AnjutaUI methods whenever
 * possible.
 */
GtkUIManager*
anjuta_ui_get_menu_merge (AnjutaUI *ui)
{
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	return ui->priv->merge;
}

/**
 * anjuta_ui_get_action:
 * @ui: This #AnjutaUI object
 * @action_group_name: Group name.
 * @action_name: Action name.
 * returns: A #GtkAction object
 *
 * Returns the action object with the name @action_name in @action_group_name.
 * Note that it will be only sucessully returned if the group has been added
 * using methods in #AnjutaUI.
 */
GtkAction*
anjuta_ui_get_action (AnjutaUI *ui, const gchar *action_group_name,
					  const gchar *action_name)
{
	GtkActionGroup *action_group;
	GtkAction *action;
	
	g_return_val_if_fail (ANJUTA_IS_UI (ui), NULL);
	
	action_group = g_hash_table_lookup (ui->priv->actions_hash,
										action_group_name);
	if (GTK_IS_ACTION_GROUP (action_group) == FALSE)
	{
		g_warning ("Unable to find action group \"%s\"", action_group_name);
		return NULL;
	}
	action = gtk_action_group_get_action (action_group, action_name);
	if (GTK_IS_ACTION (action))
		return action;
	g_warning ("Unable to find action \"%s\" in group \"%s\"",
			  action_name, action_group_name);
	return NULL;
}

/**
 * anjuta_ui_activate_action_by_path:
 * @ui: This #AnjutaUI object
 * @action_path: Path of the action in the form "GroupName/ActionName"
 *
 * Activates the action represented by @action_path. The path is in the form
 * "ActionGroupName/ActionName". Note that it will only work if the group has
 * been added using methods in #AnjutaUI.
 */
void
anjuta_ui_activate_action_by_path (AnjutaUI *ui, const gchar *action_path)
{
	const gchar *action_group_name;
	const gchar *action_name;
	GtkAction *action;
	gchar **strv;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (action_path != NULL);
	
	strv = g_strsplit (action_path, "/", 2);
	action_group_name = strv[0];
	action_name = strv[1];
	
	g_return_if_fail (action_group_name != NULL && action_name != NULL);
	
	action = anjuta_ui_get_action (ui, action_group_name, action_name);
	if (action)
		gtk_action_activate (action);
	g_strfreev (strv);
}

/**
 * anjuta_ui_activate_action_by_group:
 * @ui: This #AnjutaUI object
 * @action_group: Action group.
 * @action_name: Action name.
 *
 * Activates the action @action_name in the #GtkActionGroup @action_group.
 * "ActionGroupName/ActionName". Note that it will only work if the group has
 * been added using methods in #AnjutaUI.
 */
void
anjuta_ui_activate_action_by_group (AnjutaUI *ui, GtkActionGroup *action_group,
									const gchar *action_name)
{
	GtkAction *action;
	
	g_return_if_fail (ANJUTA_IS_UI (ui));
	g_return_if_fail (action_group != NULL && action_name != NULL);
	
	action = gtk_action_group_get_action (action_group, action_name);
	if (GTK_IS_ACTION (action))
		gtk_action_activate (action);
}

/**
 * anjuta_ui_dump_tree:
 * @ui: A #AnjutaUI object.
 *
 * Dumps the current UI XML tree in STDOUT. Useful for debugging.
 */
void
anjuta_ui_dump_tree (AnjutaUI *ui)
{
	gchar *ui_str;
	
	g_return_if_fail (ANJUTA_IS_UI(ui));
	
	gtk_ui_manager_ensure_update (ui->priv->merge);
	ui_str = gtk_ui_manager_get_ui (ui->priv->merge);
	printf("%s", ui_str);
	g_free (ui_str);
}
