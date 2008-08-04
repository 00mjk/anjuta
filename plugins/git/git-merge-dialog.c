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

#include "git-merge-dialog.h"

static void
on_merge_command_finished (AnjutaCommand *command, guint return_code,
						   Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Merge complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_merge_dialog_response (GtkDialog *dialog, gint response_id, 
						  GitBranchComboData *data)
{
	GtkWidget *merge_branch_combo;
	GtkWidget *no_commit_check;
	GtkWidget *squash_check;
	GtkWidget *use_custom_log_check;
	GtkWidget *merge_log_view;
	gchar *log;
	GtkWidget *log_prompt_dialog;
	gint prompt_response;
	GtkTreeIter iter;
	gchar *branch;
	GitMergeCommand *merge_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		merge_branch_combo = glade_xml_get_widget (data->gxml, "merge_branch_combo");
		no_commit_check = glade_xml_get_widget (data->gxml, "no_commit_check");
		squash_check = glade_xml_get_widget (data->gxml, "squash_check");
		use_custom_log_check = glade_xml_get_widget (data->gxml,
													 "use_custom_log_check");
		merge_log_view = glade_xml_get_widget (data->gxml, "merge_log_view");
		log = NULL;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (use_custom_log_check)))
		{
			log = git_get_log_from_textview (merge_log_view);
			
			if (!g_utf8_strlen (log, -1))
			{
				log_prompt_dialog = gtk_message_dialog_new (GTK_WINDOW(dialog), 
														    GTK_DIALOG_DESTROY_WITH_PARENT, 
														    GTK_MESSAGE_INFO,
														    GTK_BUTTONS_YES_NO, 
														    _("Are you sure that you want to pass an empty log message?"));
				
				prompt_response = gtk_dialog_run(GTK_DIALOG (log_prompt_dialog));
				gtk_widget_destroy (log_prompt_dialog);
				
				if (prompt_response == GTK_RESPONSE_NO)
					return;
			}
			
			g_free (log);
		}
		
		gtk_combo_box_get_active_iter (GTK_COMBO_BOX (merge_branch_combo), &iter);
		branch = git_branch_combo_model_get_branch (data->model, &iter);
		
		merge_command = git_merge_command_new (data->plugin->project_root_directory,
											   branch, log,
											   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (no_commit_check)),
											   gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (squash_check)));
		
		g_free (branch);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (merge_command), "command-finished",
						  G_CALLBACK (on_merge_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (merge_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (merge_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_branch_combo_data_free (data);
}

static void
on_use_custom_log_check_toggled (GtkToggleButton *toggle_button, GtkWidget *merge_log_view)
{
	gtk_widget_set_sensitive (merge_log_view,
							  gtk_toggle_button_get_active (toggle_button));
}

static void
merge_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *merge_branch_combo;
	GtkWidget *use_custom_log_check;
	GtkWidget *merge_log_view;
	GtkListStore *branch_list_store;
	GitBranchComboData *data;
	GitBranchListCommand *list_command;
	
	gxml = glade_xml_new (GLADE_FILE, "merge_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "merge_dialog");
	merge_branch_combo = glade_xml_get_widget (gxml, "merge_branch_combo");
	use_custom_log_check = glade_xml_get_widget (gxml, 
												 "use_custom_log_check");
	merge_log_view = glade_xml_get_widget (gxml, "merge_log_view");
	branch_list_store = git_branch_combo_model_new ();
	
	gtk_combo_box_set_model (GTK_COMBO_BOX (merge_branch_combo), 
							 GTK_TREE_MODEL (branch_list_store));
	git_branch_combo_model_setup_widget (merge_branch_combo);
	
	data = git_branch_combo_data_new (branch_list_store, 
									  GTK_COMBO_BOX (merge_branch_combo), gxml, 
									  plugin);
	
	list_command = git_branch_list_command_new (plugin->project_root_directory,
												GIT_BRANCH_TYPE_ALL);
	
	g_signal_connect (G_OBJECT (list_command), "data-arrived", 
					  G_CALLBACK (on_git_list_branch_command_data_arrived), 
					  data);
	
	g_signal_connect (G_OBJECT (list_command), "command-finished", 
					  G_CALLBACK (on_git_list_branch_command_finished), 
					  data);
	
	anjuta_command_start (ANJUTA_COMMAND (list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_merge_dialog_response), 
					  data);
	
	g_signal_connect (G_OBJECT (use_custom_log_check), "toggled",
					  G_CALLBACK (on_use_custom_log_check_toggled),
					  merge_log_view);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_merge (GtkAction *action, Git *plugin)
{
	merge_dialog (plugin);
}
