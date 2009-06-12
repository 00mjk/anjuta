/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * anjuta
 * Copyright (C) James Liggett 2008 <jrliggett@cox.net>
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

#include "git-push-dialog.h"

static void
on_push_command_finished (AnjutaCommand *command, guint return_code,
						  Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Push complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_push_dialog_response (GtkDialog *dialog, gint response_id, 
						 GitUIData *data)
{
	GtkWidget *push_origin_check;
	GtkWidget *push_url_entry;
	GtkWidget *push_all_check;
	GtkWidget *push_tags_check;
	GtkTreeModel *branch_list_model;
	gchar *url;
	GList *selected_refs;
	GitPushCommand *push_command;
	GitProgressData *progress_data;
	
	if (response_id == GTK_RESPONSE_OK)
	{
		push_origin_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                      					"push_origin_check"));
		push_url_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                     "push_url_entry"));
		push_all_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                     "push_all_check"));
		push_tags_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
		                                                      "push_tags_check"));
		branch_list_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
																	"branch_list_model"));

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_origin_check)))
			url = g_strdup ("origin");
		else 
		{
			url = gtk_editable_get_chars (GTK_EDITABLE (push_url_entry), 0, -1);
		}

		if (!git_check_input (GTK_WIDGET (dialog), push_url_entry, url, 
							  _("Please enter the URL of the repository to push to.")))
		{
			g_free (url);
			return;
		}

		selected_refs = NULL;

		gtk_tree_model_foreach (branch_list_model, 
								(GtkTreeModelForeachFunc) git_get_selected_refs,
								&selected_refs);
		
		push_command = git_push_command_new (data->plugin->project_root_directory,
		                                     url,
											 selected_refs,
		                                     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_all_check)),
		                                     gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (push_tags_check)));
		progress_data = git_progress_data_new (data->plugin, _("Git: Pushing..."));

		g_free (url);
		git_command_free_string_list (selected_refs);

		git_create_message_view (data->plugin);


		g_signal_connect (G_OBJECT (push_command), "data-arrived",
		                  G_CALLBACK (on_git_command_info_arrived),
		                  data->plugin);

		g_signal_connect (G_OBJECT (push_command), "progress",
		                  G_CALLBACK (on_git_command_progress),
		                  progress_data);

		g_signal_connect (G_OBJECT (push_command), "command-finished",
		                  G_CALLBACK (on_push_command_finished),
		                  data->plugin);
		
		g_signal_connect_swapped (G_OBJECT (push_command), "command-finished",
								  G_CALLBACK (git_progress_data_free),
								  progress_data);

		anjuta_command_start (ANJUTA_COMMAND (push_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
push_dialog (Git *plugin)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"push_dialog", "branch_list_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *push_origin_check;
	GtkWidget *push_url_entry;
	GtkListStore *branch_list_model;
	GtkCellRenderer *push_branches_view_selected_renderer;
	GitBranchListCommand *branch_list_command;
	GitUIData *data;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "push_dialog"));
	push_origin_check = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                        "push_origin_check"));
	push_url_entry = GTK_WIDGET (gtk_builder_get_object (bxml, 
	                                                     "push_url_entry"));
	branch_list_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
																"branch_list_model"));
	push_branches_view_selected_renderer = GTK_CELL_RENDERER (gtk_builder_get_object (bxml,
																					  "push_branches_view_selected_renderer"));

	branch_list_command = git_branch_list_command_new (plugin->project_root_directory,
													   GIT_BRANCH_TYPE_LOCAL);

	g_signal_connect (G_OBJECT (branch_list_command), "data-arrived",
					  G_CALLBACK (on_git_list_branch_command_data_arrived),
					  branch_list_model);

	g_signal_connect (G_OBJECT (branch_list_command), "command-finished",
					  G_CALLBACK (g_object_unref),
					  NULL);

	anjuta_command_start (ANJUTA_COMMAND (branch_list_command));
	
	data = git_ui_data_new (plugin, bxml);

	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_push_dialog_response), 
					  data);

	g_signal_connect (G_OBJECT (push_origin_check), "toggled",
	                  G_CALLBACK (on_git_origin_check_toggled),
	                  push_url_entry);

	g_signal_connect (G_OBJECT (push_branches_view_selected_renderer), "toggled",
					  G_CALLBACK (on_git_selected_column_toggled),
					  branch_list_model);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_push (GtkAction *action, Git *plugin)
{
	push_dialog (plugin);
}