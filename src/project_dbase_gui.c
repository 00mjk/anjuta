/*
    project_dbase_gui.c
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
#include <ccview.h>
#include "anjuta.h"
#include "project_dbase.h"
#include "utilities.h"
#include "resources.h"
#include "fileselection.h"

extern gchar *module_map[];

static void on_project_dbase_remove_confirm_yes_clicked (GtkButton * button,
							 gpointer user_data);
static void add_file (ProjectDBase * p);

static 
void on_project_dbase_ccview_go_to (CcviewProject* cprj, gchar* file,
			gint line, ProjectDBase* p)
{
	anjuta_goto_file_line (file, line);
}

void
on_project_add_new1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_project_view1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	ProjectDBase *p = app->project_dbase;
	g_return_if_fail (p->widgets.current_node != NULL);
	g_return_if_fail (p->current_file_data != NULL);
	anjuta_view_file (p->current_file_data->filename);
}


void
on_project_edit1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	ProjectDBase *p = app->project_dbase;
	g_return_if_fail (p->widgets.current_node != NULL);
	g_return_if_fail (p->current_file_data != NULL);
	anjuta_open_file (p->current_file_data->filename);
}

void
on_project_remove1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	gchar *buff;
	ProjectDBase *p;
	p = app->project_dbase;

	if (p->current_file_data == NULL)
		return;
	if (p->current_file_data->filename == NULL)
		return;
	buff =
		g_strdup_printf (_
				 ("Are you sure you want to remove the item\n\"%s\""
				  " from the Project Data Base?"),
extract_filename (p->current_file_data->filename));
	messagebox2 (GNOME_MESSAGE_BOX_QUESTION, buff, GNOME_STOCK_BUTTON_YES,
		     GNOME_STOCK_BUTTON_NO,
		     on_project_dbase_remove_confirm_yes_clicked, NULL, p);
	g_free (buff);
}

void
on_project_configure1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	project_config_show (app->project_dbase->project_config);
}

void
on_project_project_info1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	project_dbase_show_info(app->project_dbase);
}

void
on_project_dock_undock1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	ProjectDBase *p = app->project_dbase;
	if (p->is_docked)
		project_dbase_undock (p);
	else
		project_dbase_dock (p);
}


void
on_project_help1_activate (GtkMenuItem * menuitem, gpointer user_data)
{

}


void
on_project_include_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	app->project_dbase->sel_module = MODULE_INCLUDE;
	add_file (app->project_dbase);
}

void
on_project_source_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	app->project_dbase->sel_module = MODULE_SOURCE;
	add_file (app->project_dbase);
}

void
on_project_help_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	app->project_dbase->sel_module = MODULE_HELP;
	add_file (app->project_dbase);
}


void
on_project_data_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	app->project_dbase->sel_module = MODULE_DATA;
	add_file (app->project_dbase);
}


void
on_project_pixmap_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	app->project_dbase->sel_module = MODULE_PIXMAP;
	add_file (app->project_dbase);
}


void
on_project_translation_file1_activate (GtkMenuItem * menuitem,
				       gpointer user_data)
{
	app->project_dbase->sel_module = MODULE_PO;
	add_file (app->project_dbase);
}


void
on_project_doc_file1_activate (GtkMenuItem * menuitem, gpointer user_data)
{
	app->project_dbase->sel_module = MODULE_DOC;
	add_file (app->project_dbase);
}

static gint
on_project_dbase_win_delete_event (GtkWidget * w, GdkEvent * event,
				   gpointer data)
{
	ProjectDBase *p = data;
	project_dbase_hide (p);
	return TRUE;
}

static void
on_project_dbase_remove_confirm_yes_clicked (GtkButton * button,
					     gpointer user_data)
{
	ProjectDBase *p;
	p = user_data;
	project_dbase_remove_file (p);
}

static void
on_project_dbase_clist_select_row (GtkCList * clist,
				   gint row,
				   gint column,
				   GdkEvent * event, gpointer user_data)
{
	gchar *filename;
	GdkPixmap *pixc, *pixo;
	GdkBitmap *maskc, *masko;
	gint8 space;
	gboolean is_leaf, expanded;
	GtkCTreeNode *node;

	ProjectDBase *p = user_data;
	
	g_return_if_fail (p != NULL);
	node = gtk_ctree_node_nth (GTK_CTREE (p->widgets.ctree), row);
	p->widgets.current_node = node;
	p->current_file_data =
		gtk_ctree_node_get_row_data (GTK_CTREE (p->widgets.ctree),
					     GTK_CTREE_NODE (node));
	gtk_ctree_get_node_info (GTK_CTREE (p->widgets.ctree),
				 node,
				 &filename, &space,
				 &pixc, &maskc, &pixo, &masko, &is_leaf,
				 &expanded);

	if (event == NULL)
		return;
	if (event->type == GDK_2BUTTON_PRESS) {
		if (((GdkEventButton *) event)->button == 1) {
			on_project_edit1_activate (NULL, NULL);
			return;
		}
	} else if (event->type == GDK_KEY_PRESS) {
		if (((GdkEventKey *) event)->keyval == GDK_Return)
			on_project_edit1_activate (NULL, NULL);
	}
}

static void
on_project_dbase_close_load_yes_clicked (GtkButton * b, gpointer data)
{
	ProjectDBase *p = data;
	gtk_widget_hide (app->project_dbase->fileselection_open);
	project_dbase_close_project (p);
	project_dbase_load_project (p, TRUE);
}

void
on_open_prjfilesel_ok_clicked (GtkButton * button, gpointer user_data)
{
	ProjectDBase *p = app->project_dbase;
	if (p->project_is_open)
	{
		messagebox2 (GNOME_MESSAGE_BOX_WARNING,
			     _("There is already a project opened." \
			       "Do you want to close it first?"),
			     GNOME_STOCK_BUTTON_YES,
			     GNOME_STOCK_BUTTON_NO,
			     GTK_SIGNAL_FUNC
			     (on_project_dbase_close_load_yes_clicked), NULL,
			     p);
		return;
	}
	gtk_widget_hide (p->fileselection_open);
	project_dbase_load_project (p, TRUE);
}

void
on_open_prjfilesel_cancel_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->project_dbase->fileselection_open);
}

static gboolean
on_project_dbase_event (GtkWidget * widget,
			GdkEvent * event, gpointer user_data)
{
	gint row;
	GtkCTree *tree;
	GtkCTreeNode *node;
	GdkEventButton *bevent;
	ProjectDBase *pd = app->project_dbase;

	if (event->type == GDK_BUTTON_PRESS) {
		if (((GdkEventButton *) event)->button != 3)
			return FALSE;
		bevent = (GdkEventButton *) event;
		bevent->button = 1;
		project_dbase_update_controls (pd);
	
		/* Popup project menu */
		gtk_menu_popup (GTK_MENU (pd->widgets.menu), NULL,
				NULL, NULL, NULL, bevent->button, bevent->time);
	
		return TRUE;
	} else if (event->type == GDK_KEY_PRESS){
		tree = GTK_CTREE(widget);
		row = tree->clist.focus_row;
		node = gtk_ctree_node_nth(tree,row);
		
		switch(((GdkEventKey *)event)->keyval) {
			case GDK_Return:
				if(GTK_CTREE_ROW(node)->is_leaf)
					on_project_dbase_clist_select_row (GTK_CLIST(&tree->clist),row,-1,event,user_data);
				break;
			case GDK_Left:
				gtk_ctree_collapse(tree, node);
				break;
			case GDK_Right:
				gtk_ctree_expand(tree, node);
				break;
			default:
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

static void
add_file (ProjectDBase * p)
{
	gchar *title;

	g_return_if_fail (p != NULL);

	title =
		g_strconcat ("Add file to module: ",
			     module_map[p->sel_module], NULL);
	gtk_window_set_title (GTK_WINDOW (p->fileselection_add_file), title);
	g_free (title);
	gtk_widget_show (p->fileselection_add_file);
}

static GnomeUIInfo import_file1_menu_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Include file"),
	 NULL,
	 on_project_include_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Source file"),
	 NULL,
	 on_project_source_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Help file"),
	 NULL,
	 on_project_help_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Data file"),
	 NULL,
	 on_project_data_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Pixmap file"),
	 NULL,
	 on_project_pixmap_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Translation file"),
	 NULL,
	 on_project_translation_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Doc file"),
	 NULL,
	 on_project_doc_file1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static GnomeUIInfo menu1_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Add New"),
	 NULL,
	 on_project_add_new1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_SUBTREE, N_("Import File"),
	 NULL,
	 import_file1_menu_uiinfo, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("View"),
	 NULL,
	 on_project_view1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Edit"),
	 NULL,
	 on_project_edit1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Remove"),
	 NULL,
	 on_project_remove1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Configure Project"),
	 NULL,
	 on_project_configure1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Project Info"),
	 NULL,
	 on_project_project_info1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Dock/Undock"),
	 NULL,
	 on_project_dock_undock1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	{
	 GNOME_APP_UI_ITEM, N_("Help"),
	 NULL,
	 on_project_help1_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

static void
create_project_menus (ProjectDBase * p)
{
	GtkWidget *menu;

	menu = gtk_menu_new ();
	gnome_app_fill_menu (GTK_MENU_SHELL (menu), menu1_uiinfo,
			     NULL, FALSE, 0);
	p->widgets.menu = menu;
	p->widgets.menu_import = menu1_uiinfo[1].widget;
	p->widgets.menu_view = menu1_uiinfo[3].widget;
	p->widgets.menu_edit = menu1_uiinfo[4].widget;
	p->widgets.menu_remove = menu1_uiinfo[6].widget;
	p->widgets.menu_configure = menu1_uiinfo[8].widget;
	p->widgets.menu_info = menu1_uiinfo[9].widget;
	
	/* unimplemented*/
	gtk_widget_hide (import_file1_menu_uiinfo[0].widget);
	gtk_widget_hide (menu1_uiinfo[0].widget);
	
	gtk_widget_ref (menu);
}

void
create_project_dbase_gui (ProjectDBase * p)
{
	GtkWidget *window1;
	GtkWidget *eventbox1;
	GtkWidget *notebook1;
	GtkWidget *scrolledwindow1;
	GtkWidget *ccview_prj;
	GtkWidget *ctree1;
	GtkCList *clist1;

	window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window1), _("Project: None"));
	gtk_window_set_wmclass (GTK_WINDOW (window1), "project_dbase", "Anjuta");

	eventbox1 = gtk_event_box_new ();
	gtk_widget_show (eventbox1);
	gtk_container_add (GTK_CONTAINER (window1), eventbox1);
	
	ccview_prj = ccview_project_new();
	gtk_widget_show (ccview_prj);
	notebook1 = CCVIEW_PROJECT (ccview_prj)->notebook;
	ccview_project_set_recursive (CCVIEW_PROJECT(ccview_prj), TRUE);
	ccview_project_set_use_automake (CCVIEW_PROJECT(ccview_prj), FALSE);
	ccview_project_set_follow_includes (CCVIEW_PROJECT(ccview_prj), FALSE);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show (scrolledwindow1);
    gtk_notebook_prepend_page(GTK_NOTEBOOK(notebook1),scrolledwindow1,
                             gtk_label_new(_("Project")));
	gtk_notebook_set_page(GTK_NOTEBOOK(notebook1), 0);

	ctree1 = gtk_ctree_new (1, 0);
	clist1 = &(GTK_CTREE (ctree1)->clist);
	gtk_clist_set_column_auto_resize (GTK_CLIST (ctree1), 0, TRUE);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), ctree1);
	gtk_clist_set_selection_mode (clist1, GTK_SELECTION_BROWSE);
	gtk_ctree_set_line_style (GTK_CTREE(ctree1), GTK_CTREE_LINES_DOTTED);
	gtk_widget_show (ctree1);

	gtk_accel_group_attach (app->accel_group, GTK_OBJECT (window1));

	gtk_signal_connect (GTK_OBJECT (window1), "delete_event",
			    GTK_SIGNAL_FUNC
			    (on_project_dbase_win_delete_event), p);
	gtk_signal_connect (GTK_OBJECT (clist1), "select_row",
			    GTK_SIGNAL_FUNC
			    (on_project_dbase_clist_select_row), p);
	gtk_signal_connect (GTK_OBJECT (ctree1), "event",
			    GTK_SIGNAL_FUNC (on_project_dbase_event), p);

	gtk_signal_connect (GTK_OBJECT (ccview_prj), "go_to",
			    GTK_SIGNAL_FUNC (on_project_dbase_ccview_go_to), p);

	p->widgets.window = window1;
	p->widgets.ccview = ccview_prj;
	p->widgets.client_area = eventbox1;
	p->widgets.client = ccview_prj;
	p->widgets.ctree = ctree1;
	p->widgets.scrolledwindow = scrolledwindow1;

	create_project_menus (p);

	gtk_widget_ref (p->widgets.window);
	gtk_widget_ref (p->widgets.ccview);
	gtk_widget_ref (p->widgets.client_area);
	gtk_widget_ref (p->widgets.client);
	gtk_widget_ref (p->widgets.scrolledwindow);
	gtk_widget_ref (p->widgets.ctree);
}

GtkWidget *
create_project_dbase_info_gui (gchar * lab[])
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *frame1;
	GtkWidget *table1;
	GtkWidget *frame2;
	GtkWidget *label2;
	GtkWidget *frame3;
	GtkWidget *label4;
	GtkWidget *label5;
	GtkWidget *label1;
	GtkWidget *frame7;
	GtkWidget *label12;
	GtkWidget *frame8;
	GtkWidget *label14;
	GtkWidget *frame9;
	GtkWidget *label16;
	GtkWidget *frame10;
	GtkWidget *label18;
	GtkWidget *frame11;
	GtkWidget *label20;
	GtkWidget *label11;
	GtkWidget *label13;
	GtkWidget *label15;
	GtkWidget *label17;
	GtkWidget *label19;
	GtkWidget *label21;
	GtkWidget *frame12;
	GtkWidget *label22;
	GtkWidget *label23;
	GtkWidget *label3;
	GtkWidget *frame4;
	GtkWidget *label6;
	GtkWidget *label24;
	GtkWidget *frame13;
	GtkWidget *label25;
	GtkWidget *label26;
	GtkWidget *frame14;
	GtkWidget *label27;
	GtkWidget *frame15;
	GtkWidget *label28;
	GtkWidget *vseparator1;
	GtkWidget *label29;
	GtkWidget *frame16;
	GtkWidget *label30;
	GtkWidget *hseparator1;
	GtkWidget *hseparator2;
	GtkWidget *hseparator3;
	GtkWidget *hseparator4;
	GtkWidget *hseparator5;
	GtkWidget *hseparator6;
	GtkWidget *label31;
	GtkWidget *frame17;
	GtkWidget *label32;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;

	dialog1 = gnome_dialog_new (_("Anjuta: Project Information"), NULL);
	gtk_window_set_wmclass (GTK_WINDOW (dialog1), "proj_info", "Anjuta");
	gnome_dialog_set_close (GNOME_DIALOG (dialog1), TRUE);

	dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox1);

	frame1 = gtk_frame_new (_(" Project Information "));
	gtk_widget_show (frame1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), frame1, TRUE, TRUE, 0);

	table1 = gtk_table_new (11, 5, FALSE);
	gtk_widget_show (table1);
	gtk_container_add (GTK_CONTAINER (frame1), table1);

	frame2 = gtk_frame_new (NULL);
	gtk_widget_show (frame2);
	gtk_table_attach (GTK_TABLE (table1), frame2, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame2), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);

	label2 = gtk_label_new (lab[0]);
	gtk_widget_show (label2);
	gtk_container_add (GTK_CONTAINER (frame2), label2);
	gtk_misc_set_padding (GTK_MISC (label2), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label2), 0, -1);

	frame3 = gtk_frame_new (NULL);
	gtk_widget_show (frame3);
	gtk_table_attach (GTK_TABLE (table1), frame3, 1, 2, 4, 5,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame3), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_IN);

	label4 = gtk_label_new (lab[1]);
	gtk_widget_show (label4);
	gtk_container_add (GTK_CONTAINER (frame3), label4);
	gtk_misc_set_padding (GTK_MISC (label4), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, -1);

	label5 = gtk_label_new (_("Author:"));
	gtk_widget_show (label5);
	gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 5, 6,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_padding (GTK_MISC (label5), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label5), 0, -1);

	label1 = gtk_label_new (_("Project Name:"));
	gtk_widget_show (label1);
	gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 2, 3,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
	gtk_misc_set_padding (GTK_MISC (label1), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label1), 0, -1);

	frame7 = gtk_frame_new (NULL);
	gtk_widget_show (frame7);
	gtk_table_attach (GTK_TABLE (table1), frame7, 4, 5, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame7), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame7), GTK_SHADOW_IN);

	label12 = gtk_label_new (lab[6]);
	gtk_widget_show (label12);
	gtk_container_add (GTK_CONTAINER (frame7), label12);
	gtk_misc_set_padding (GTK_MISC (label12), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label12), 0, -1);

	frame8 = gtk_frame_new (NULL);
	gtk_widget_show (frame8);
	gtk_table_attach (GTK_TABLE (table1), frame8, 4, 5, 4, 5,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame8), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame8), GTK_SHADOW_IN);

	label14 = gtk_label_new (lab[7]);
	gtk_widget_show (label14);
	gtk_container_add (GTK_CONTAINER (frame8), label14);
	gtk_misc_set_padding (GTK_MISC (label14), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label14), 0, -1);

	frame9 = gtk_frame_new (NULL);
	gtk_widget_show (frame9);
	gtk_table_attach (GTK_TABLE (table1), frame9, 4, 5, 5, 6,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame9), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame9), GTK_SHADOW_IN);

	label16 = gtk_label_new (lab[8]);
	gtk_widget_show (label16);
	gtk_container_add (GTK_CONTAINER (frame9), label16);
	gtk_misc_set_padding (GTK_MISC (label16), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label16), 0, -1);

	frame10 = gtk_frame_new (NULL);
	gtk_widget_show (frame10);
	gtk_table_attach (GTK_TABLE (table1), frame10, 4, 5, 6, 7,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame10), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame10), GTK_SHADOW_IN);

	label18 = gtk_label_new (lab[9]);
	gtk_widget_show (label18);
	gtk_container_add (GTK_CONTAINER (frame10), label18);
	gtk_misc_set_padding (GTK_MISC (label18), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label18), 0, -1);

	frame11 = gtk_frame_new (NULL);
	gtk_widget_show (frame11);
	gtk_table_attach (GTK_TABLE (table1), frame11, 4, 5, 8, 9,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame11), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame11), GTK_SHADOW_IN);

	label20 = gtk_label_new (lab[10]);
	gtk_widget_show (label20);
	gtk_container_add (GTK_CONTAINER (frame11), label20);
	gtk_misc_set_padding (GTK_MISC (label20), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label20), 0, -1);

	label11 = gtk_label_new (_("Program Name:"));
	gtk_widget_show (label11);
	gtk_table_attach (GTK_TABLE (table1), label11, 3, 4, 2, 3,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label11), 0, -1);

	label13 = gtk_label_new (_("No of src files:"));
	gtk_widget_show (label13);
	gtk_table_attach (GTK_TABLE (table1), label13, 3, 4, 4, 5,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label13), 0, -1);

	label15 = gtk_label_new (_("No of help files::"));
	gtk_widget_show (label15);
	gtk_table_attach (GTK_TABLE (table1), label15, 3, 4, 5, 6,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label15), 0, -1);

	label17 = gtk_label_new (_("No of data files:"));
	gtk_widget_show (label17);
	gtk_table_attach (GTK_TABLE (table1), label17, 3, 4, 6, 7,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label17), 0, -1);

	label19 = gtk_label_new (_("No of pix files:"));
	gtk_widget_show (label19);
	gtk_table_attach (GTK_TABLE (table1), label19, 3, 4, 8, 9,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label19), 0, -1);

	label21 = gtk_label_new (_("No of doc files:"));
	gtk_widget_show (label21);
	gtk_table_attach (GTK_TABLE (table1), label21, 3, 4, 9, 10,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label21), 0, -1);

	frame12 = gtk_frame_new (NULL);
	gtk_widget_show (frame12);
	gtk_table_attach (GTK_TABLE (table1), frame12, 4, 5, 9, 10,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame12), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame12), GTK_SHADOW_IN);

	label22 = gtk_label_new (lab[11]);
	gtk_widget_show (label22);
	gtk_container_add (GTK_CONTAINER (frame12), label22);
	gtk_misc_set_padding (GTK_MISC (label22), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label22), 0, -1);

	label23 = gtk_label_new (_("No of PO files:"));
	gtk_widget_show (label23);
	gtk_table_attach (GTK_TABLE (table1), label23, 3, 4, 10, 11,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label23), 0, -1);

	label3 = gtk_label_new (_("Version:"));
	gtk_widget_show (label3);
	gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 4, 5,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_LEFT);
	gtk_misc_set_padding (GTK_MISC (label3), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label3), 0, -1);

	frame4 = gtk_frame_new (NULL);
	gtk_widget_show (frame4);
	gtk_table_attach (GTK_TABLE (table1), frame4, 1, 2, 5, 7,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame4), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame4), GTK_SHADOW_IN);

	label6 = gtk_label_new (lab[2]);
	gtk_widget_show (label6);
	gtk_container_add (GTK_CONTAINER (frame4), label6);
	gtk_misc_set_padding (GTK_MISC (label6), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label6), 0, -1);

	label24 = gtk_label_new (_("Gui editable by Glade:"));
	gtk_widget_show (label24);
	gtk_table_attach (GTK_TABLE (table1), label24, 0, 1, 8, 9,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_padding (GTK_MISC (label24), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label24), 0, -1);

	frame13 = gtk_frame_new (NULL);
	gtk_widget_show (frame13);
	gtk_table_attach (GTK_TABLE (table1), frame13, 1, 2, 8, 9,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame13), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame13), GTK_SHADOW_IN);

	label25 = gtk_label_new (lab[3]);
	gtk_widget_show (label25);
	gtk_container_add (GTK_CONTAINER (frame13), label25);
	gtk_misc_set_padding (GTK_MISC (label25), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label25), 0, -1);

	label26 = gtk_label_new (_("Makefiles managed:"));
	gtk_widget_show (label26);
	gtk_table_attach (GTK_TABLE (table1), label26, 0, 1, 9, 10,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_padding (GTK_MISC (label26), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label26), 0, -1);

	frame14 = gtk_frame_new (NULL);
	gtk_widget_show (frame14);
	gtk_table_attach (GTK_TABLE (table1), frame14, 1, 2, 9, 10,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame14), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame14), GTK_SHADOW_IN);

	label27 = gtk_label_new (lab[4]);
	gtk_widget_show (label27);
	gtk_container_add (GTK_CONTAINER (frame14), label27);
	gtk_misc_set_padding (GTK_MISC (label27), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label27), 0, -1);

	frame15 = gtk_frame_new (NULL);
	gtk_widget_show (frame15);
	gtk_table_attach (GTK_TABLE (table1), frame15, 4, 5, 10, 11,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame15), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame15), GTK_SHADOW_IN);

	label28 = gtk_label_new (lab[12]);
	gtk_widget_show (label28);
	gtk_container_add (GTK_CONTAINER (frame15), label28);
	gtk_misc_set_padding (GTK_MISC (label28), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label28), 0, -1);

	vseparator1 = gtk_vseparator_new ();
	gtk_widget_show (vseparator1);
	gtk_table_attach (GTK_TABLE (table1), vseparator1, 2, 3, 1, 11,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 3, 2);

	label29 = gtk_label_new (_("Gettext support:"));
	gtk_widget_show (label29);
	gtk_table_attach (GTK_TABLE (table1), label29, 0, 1, 10, 11,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_padding (GTK_MISC (label29), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label29), 0, -1);

	frame16 = gtk_frame_new (NULL);
	gtk_widget_show (frame16);
	gtk_table_attach (GTK_TABLE (table1), frame16, 1, 2, 10, 11,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame16), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame16), GTK_SHADOW_IN);

	label30 = gtk_label_new (lab[5]);
	gtk_widget_show (label30);
	gtk_container_add (GTK_CONTAINER (frame16), label30);
	gtk_misc_set_padding (GTK_MISC (label30), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label30), 0, -1);

	hseparator1 = gtk_hseparator_new ();
	gtk_widget_show (hseparator1);
	gtk_table_attach (GTK_TABLE (table1), hseparator1, 0, 2, 7, 8,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	hseparator2 = gtk_hseparator_new ();
	gtk_widget_show (hseparator2);
	gtk_table_attach (GTK_TABLE (table1), hseparator2, 0, 2, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	hseparator3 = gtk_hseparator_new ();
	gtk_widget_show (hseparator3);
	gtk_table_attach (GTK_TABLE (table1), hseparator3, 0, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	hseparator4 = gtk_hseparator_new ();
	gtk_widget_show (hseparator4);
	gtk_table_attach (GTK_TABLE (table1), hseparator4, 3, 5, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	hseparator5 = gtk_hseparator_new ();
	gtk_widget_show (hseparator5);
	gtk_table_attach (GTK_TABLE (table1), hseparator5, 3, 5, 3, 4,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	hseparator6 = gtk_hseparator_new ();
	gtk_widget_show (hseparator6);
	gtk_table_attach (GTK_TABLE (table1), hseparator6, 3, 5, 7, 8,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	label31 = gtk_label_new (_("Project Type:"));
	gtk_widget_show (label31);
	gtk_table_attach (GTK_TABLE (table1), label31, 0, 1, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label31), 0, -1);

	frame17 = gtk_frame_new (NULL);
	gtk_widget_show (frame17);
	gtk_table_attach (GTK_TABLE (table1), frame17, 1, 5, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_container_set_border_width (GTK_CONTAINER (frame17), 3);
	gtk_frame_set_shadow_type (GTK_FRAME (frame17), GTK_SHADOW_IN);

	label32 = gtk_label_new (lab[13]);
	gtk_widget_show (label32);
	gtk_container_add (GTK_CONTAINER (frame17), label32);
	gtk_misc_set_padding (GTK_MISC (label32), 3, 0);
	gtk_misc_set_alignment (GTK_MISC (label32), 0, -1);

	dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_CLOSE);
	button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

	return dialog1;
}

GtkWidget *
create_project_confirm_dlg ()
{
	GtkWidget *mesgbox;
	GtkWidget *dialog_vbox9;

	mesgbox = gnome_message_box_new (
		_("Project is not saved.\n"
		"Do you want to save it before closing?"),
	       GNOME_MESSAGE_BOX_QUESTION, NULL);
	gtk_window_set_policy (GTK_WINDOW (mesgbox), FALSE, FALSE, FALSE);
	dialog_vbox9 = GNOME_DIALOG (mesgbox)->vbox;
	gtk_widget_show (dialog_vbox9);
	gnome_dialog_append_button (GNOME_DIALOG (mesgbox),
				    GNOME_STOCK_BUTTON_YES);
	gnome_dialog_append_button (GNOME_DIALOG (mesgbox),
				    GNOME_STOCK_BUTTON_NO);
	gnome_dialog_append_button (GNOME_DIALOG (mesgbox),
				    GNOME_STOCK_BUTTON_CANCEL);
	return mesgbox;
}

#if 0 /* Disabling */
static gchar *lang_data[] = {
	N_("Breton"), "br",
	N_("Catalan"), "ca",
	N_("Czech"), "cs",
	N_("Danish"), "da",
	N_("German"), "de",
	N_("Greek"), "el",
	N_("Esperanto"), "eo",
	N_("Spanish"), "es",
	N_("Finnish"), "fi",
	N_("French"), "fr",
	N_("Herbrew"), "he",
	N_("Croatian"), "hr",
	N_("Hungarian"), "hu",
	N_("Islandic"), "is",
	N_("Italian"), "it",
	N_("Croatian"), "hr",
	N_("Korean"), "ko",
	N_("Macedonian"), "mk",
	N_("Dutch"), "nl",
	N_("Norwegian"), "no",
	N_("Polish"), "pl",
	N_("Portuguese"), "pt_BR",
	N_("Romanian"), "ro",
	N_("Russian"), "ru",
	N_("Slovak"), "sk",
	N_("Turkish"), "tr",
	N_("Simplified Chinese"), "zh_CN.GB2312",
	N_("Chinese"), "zh_TW.Big5",
	N_("Estonian"), "et",
	NULL
};

static gchar *
get_language ()
{
	GtkWidget *dlg;
	gint but;

	dlg = create_langsel_dialog ();
	but = gnome_dialog_run (GNOME_DIALOG (dlg));
	if (but == 0)
		return NULL;
	return
		gtk_object_get_data (GTK_OBJECT
				     (lookup_widget (dlg, "combo_entry1")),
				     "language_key");
}


static void
on_lang_combo_entry_changed (GtkEditable * editable, gpointer user_data)
{

}

GtkWidget *
create_langsel_dialog (void)
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *table1;
	GtkWidget *label1;
	GtkWidget *combo1;
	GList *combo1_items = NULL;
	GtkWidget *combo_entry1;
	gchar *pixmap1_filename;
	GtkWidget *pixmap1;
	GtkWidget *dialog_action_area1;
	GtkWidget *button1;
	GtkWidget *button3;
	gint i;

	dialog1 = gnome_dialog_new (_("Select regional language"), NULL);
	gtk_object_set_data (GTK_OBJECT (dialog1), "dialog1", dialog1);
	gtk_window_set_policy (GTK_WINDOW (dialog1), FALSE, FALSE, FALSE);

	dialog_vbox1 = GNOME_DIALOG (dialog1)->vbox;
	gtk_object_set_data (GTK_OBJECT (dialog1), "dialog_vbox1",
			     dialog_vbox1);
	gtk_widget_show (dialog_vbox1);

	table1 = gtk_table_new (2, 2, FALSE);
	gtk_widget_ref (table1);
	gtk_object_set_data_full (GTK_OBJECT (dialog1), "table1", table1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), table1, TRUE, TRUE, 0);

	label1 = gtk_label_new (_("Select Regional Language:"));
	gtk_widget_ref (label1);
	gtk_object_set_data_full (GTK_OBJECT (dialog1), "label1", label1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label1);
	gtk_table_attach (GTK_TABLE (table1), label1, 1, 2, 0, 1,
			  (GtkAttachOptions) (0),
			  (GtkAttachOptions) (0), 0, 0);

	combo1 = gtk_combo_new ();
	gtk_widget_ref (combo1);
	gtk_object_set_data_full (GTK_OBJECT (dialog1), "combo1", combo1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (combo1);
	gtk_table_attach (GTK_TABLE (table1), combo1, 1, 2, 1, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	for (i = 0;; i += 2)
	{
		if (lang_data[i] == NULL)
			break;
		combo1_items = g_list_append (combo1_items, _(lang_data[i]));
	}
	gtk_combo_set_popdown_strings (GTK_COMBO (combo1), combo1_items);
	g_list_free (combo1_items);

	combo_entry1 = GTK_COMBO (combo1)->entry;
	gtk_widget_ref (combo_entry1);
	gtk_object_set_data_full (GTK_OBJECT (dialog1), "combo_entry1",
				  combo_entry1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (combo_entry1);
	gtk_entry_set_text (GTK_ENTRY (combo_entry1), _("Breton"));

	pixmap1 = gtk_type_new (gnome_pixmap_get_type ());
	pixmap1_filename = gnome_pixmap_file ("project7/gnome-globe.png");
	if (pixmap1_filename)
		gnome_pixmap_load_file (GNOME_PIXMAP (pixmap1),
					pixmap1_filename);
	else
		g_warning (_("Couldn't find pixmap file: %s"),
			   "gnome-globe.png");
	g_free (pixmap1_filename);
	gtk_widget_ref (pixmap1);
	gtk_object_set_data_full (GTK_OBJECT (dialog1), "pixmap1", pixmap1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (pixmap1);
	gtk_table_attach (GTK_TABLE (table1), pixmap1, 0, 1, 0, 2,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (GTK_FILL), 0, 0);

	dialog_action_area1 = GNOME_DIALOG (dialog1)->action_area;
	gtk_object_set_data (GTK_OBJECT (dialog1), "dialog_action_area1",
			     dialog_action_area1);
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
				   GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog_action_area1), 8);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_OK);
	button1 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_ref (button1);
	gtk_object_set_data_full (GTK_OBJECT (dialog1), "button1", button1,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button1);
	GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);

	gnome_dialog_append_button (GNOME_DIALOG (dialog1),
				    GNOME_STOCK_BUTTON_CANCEL);
	button3 = g_list_last (GNOME_DIALOG (dialog1)->buttons)->data;
	gtk_widget_ref (button3);
	gtk_object_set_data_full (GTK_OBJECT (dialog1), "button3", button3,
				  (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (button3);
	GTK_WIDGET_SET_FLAGS (button3, GTK_CAN_DEFAULT);

	gtk_signal_connect (GTK_OBJECT (combo_entry1), "changed",
			    GTK_SIGNAL_FUNC (on_lang_combo_entry_changed),
			    NULL);

	return dialog1;
}
#endif /* Disabling */

static void
on_prj_import_confirm_yes (GtkButton * button, gpointer user_data)
{
	gchar *filename, *dir, *comp_dir;
	GList *list, *mod_files;

	ProjectDBase *p = user_data;

	gtk_widget_hide (p->fileselection_add_file);

	filename =  fileselection_get_filename (p->fileselection_add_file);
	if (!filename)
		return;
	dir = g_dirname (filename);

	comp_dir = project_dbase_get_module_dir (p, p->sel_module);
	mod_files = project_dbase_get_module_files (p, p->sel_module);
	list = mod_files;
	while (list)
	{
		if (strcmp (extract_filename (filename), list->data) == 0)
		{
			/*
			 * file has already been added. So skip with a message 
			 */
			messagebox (GNOME_MESSAGE_BOX_INFO,
				    _
				    ("This file has already been added to the project."));
			g_free (dir);
			g_free (comp_dir);
			g_free (filename);
			glist_strings_free (mod_files);
			return;
		}
		list = g_list_next (list);
	}
	glist_strings_free (mod_files);
	/*
	 * File has not been added. So add it 
	 */
	if (strcmp (dir, comp_dir) != 0)
	{
		gchar* fn;
		/*
		 * File does not exist in the corrospondig dir. So, import it. 
		 */
		fn =
			g_strconcat (comp_dir, "/",
				     extract_filename (filename), NULL);
		force_create_dir (comp_dir);
		if (!copy_file (filename, fn, TRUE))
		{
			g_free (dir);
			g_free (comp_dir);
			g_free (fn);
			g_free (filename);
			messagebox (GNOME_MESSAGE_BOX_INFO,
				    _("Error while copying the file inside the module."));
			return;
		}
		g_free(fn);
	}
	project_dbase_add_file_to_module (p, p->sel_module, filename);
	g_free (dir);
	g_free (comp_dir);
	g_free (filename);
	return;
}

void
on_add_prjfilesel_cancel_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (app->project_dbase->fileselection_add_file);
}

void
on_add_prjfilesel_ok_clicked (GtkButton * button, gpointer user_data)
{
	gchar *filename, *dir, *comp_dir, *mesg;
	ProjectDBase *p = user_data;

	filename =  fileselection_get_filename (p->fileselection_add_file);
	if (!filename)
		return;
	if (file_is_regular (filename) == FALSE)
	{
		anjuta_error (_("Not a regular file: %s."), filename);
		g_free (filename);
		return;
	}
	dir = g_dirname (filename);
	comp_dir = project_dbase_get_module_dir (p, p->sel_module);
	mesg =
		g_strdup_printf (_
				 ("\"%s\"\ndoes not exist in the current module directory."
				  "\nDo you want to IMPORT (ie copy) into the module?"),
				filename);
	if (strcmp (dir, comp_dir) == 0)
		on_prj_import_confirm_yes (NULL, user_data);
	else
		messagebox2 (GNOME_MESSAGE_BOX_INFO, mesg,
			     GNOME_STOCK_BUTTON_YES,
			     GNOME_STOCK_BUTTON_NO,
			     on_prj_import_confirm_yes, NULL, user_data);
	gtk_widget_hide (p->fileselection_add_file);
	g_free (dir);
	g_free (mesg);
	g_free (comp_dir);
	g_free (filename);
}
