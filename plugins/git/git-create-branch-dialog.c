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

#include "git-create-branch-dialog.h"

static void
on_create_command_finished (AnjutaCommand *command, guint return_code,
							Git *plugin)
{
	AnjutaStatus *status;
	gchar *branch_name;
	gchar *status_message;
	
	if (return_code == 0)
	{
		status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
										  NULL);
		
		branch_name = git_branch_create_command_get_branch_name (GIT_BRANCH_CREATE_COMMAND (command));
		status_message = g_strdup_printf (_("Git: Created branch \"%s\"."), 
										  branch_name);
		anjuta_status (status, status_message, 5);
		
		g_free (branch_name);
		g_free (status_message);
	}
	
	
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_create_branch_dialog_response (GtkDialog *dialog, gint response_id, 
								  GitUIData *data)
{
	GtkWidget *branch_name_entry;
	GtkWidget *branch_checkout_check;
	GtkWidget *branch_revision_radio;
	GtkWidget *branch_revision_entry;
	GtkWidget *branch_branch_radio;
	GtkWidget *branch_branch_combo;
	GtkTreeModel *branch_combo_model;
	gchar *branch_name;
	gchar *revision;
	GtkTreeIter iter;
	gboolean checkout;
	GitBranchCreateCommand *create_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		branch_name_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																"branch_name_entry"));
		branch_checkout_check = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																	"branch_checkout_check"));
		branch_revision_radio = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																	"branch_revision_radio"));
		branch_revision_entry = GTK_WIDGET (gtk_builder_get_object (data->bxml, 
																    "branch_revision_entry"));
		branch_branch_radio = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																  "branch_branch_radio"));
		branch_branch_combo = GTK_WIDGET (gtk_builder_get_object (data->bxml,
																  "branch_branch_combo"));
		branch_combo_model = GTK_TREE_MODEL (gtk_builder_get_object (data->bxml,
																	 "branch_combo_model"));
		branch_name = gtk_editable_get_chars (GTK_EDITABLE (branch_name_entry),
											  0, -1);
		revision = NULL;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (branch_revision_radio)))
		{
			revision = gtk_editable_get_chars (GTK_EDITABLE (branch_revision_entry),
											   0, -1);
			if (!git_check_input (GTK_WIDGET (dialog), branch_revision_entry, 
								  revision, _("Please enter a revision.")))
			{
				g_free (revision);
				g_free (branch_name);
				return;
			}
		}
		else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (branch_branch_radio)))
		{
			if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (branch_branch_combo), 
											   &iter))
			{
				gtk_tree_model_get (branch_combo_model, &iter, 0, &revision, 
									-1);
			}
		}
		
		if (!git_check_input (GTK_WIDGET (dialog), branch_revision_entry, 
							  branch_name, _("Please enter a branch name.")))
		{
			
			g_free (revision);
			g_free (branch_name);
			return;
		}
		
		checkout = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (branch_checkout_check));
		
		create_command = git_branch_create_command_new (data->plugin->project_root_directory,
														branch_name,
														revision,
														checkout);
		
		g_free (branch_name);
		g_free (revision);
		
		git_create_message_view (data->plugin);
		
		g_signal_connect (G_OBJECT (create_command), "command-finished",
						  G_CALLBACK (on_create_command_finished),
						  data->plugin);
		
		g_signal_connect (G_OBJECT (create_command), "data-arrived",
						  G_CALLBACK (on_git_command_info_arrived),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (create_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
on_branch_revision_radio_toggled (GtkToggleButton *toggle_button, 
								  GtkWidget *widget)
{
	GtkWidget *create_branch_dialog;
	gboolean active;
	
	create_branch_dialog = gtk_widget_get_toplevel (GTK_WIDGET (toggle_button));
	
	active = gtk_toggle_button_get_active (toggle_button);
	gtk_widget_set_sensitive (widget, active);
	
	if (active && GTK_WIDGET_CAN_FOCUS (widget))
	{
		gtk_window_set_focus (GTK_WINDOW (create_branch_dialog),
							  widget);
	}
	
}

static void
create_branch_dialog (Git *plugin, const gchar *revision)
{
	GtkBuilder *bxml;
	gchar *objects[] = {"create_branch_dialog", "branch_combo_model", NULL};
	GError *error;
	GtkWidget *dialog;
	GtkWidget *branch_revision_radio;
	GtkWidget *branch_revision_entry;
	GtkWidget *branch_branch_radio;
	GtkWidget *branch_branch_combo;
	GtkListStore *branch_combo_model;
	GitUIData *data;
	GitBranchListCommand *branch_list_command;
	
	bxml = gtk_builder_new ();
	error = NULL;

	if (!gtk_builder_add_objects_from_file (bxml, BUILDER_FILE, objects, 
	                                        &error))
	{
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}
	
	dialog = GTK_WIDGET (gtk_builder_get_object (bxml, "create_branch_dialog"));
	branch_revision_radio = GTK_WIDGET (gtk_builder_get_object (bxml, 
															    "branch_revision_radio"));
	branch_revision_entry = GTK_WIDGET (gtk_builder_get_object (bxml, 
																"branch_revision_entry"));
	branch_branch_radio = GTK_WIDGET (gtk_builder_get_object (bxml,
															  "branch_branch_radio"));
	branch_branch_combo = GTK_WIDGET (gtk_builder_get_object (bxml,
															  "branch_branch_combo"));
	branch_combo_model = GTK_LIST_STORE (gtk_builder_get_object (bxml,
																 "branch_combo_model"));
	data = git_ui_data_new (plugin, bxml);

	branch_list_command = git_branch_list_command_new (plugin->project_root_directory,
													   GIT_BRANCH_TYPE_ALL);

	g_signal_connect (G_OBJECT (branch_list_command), "data-arrived",
					  G_CALLBACK (on_git_list_branch_combo_command_data_arrived),
					  branch_combo_model);

	g_signal_connect (G_OBJECT (branch_list_command), "command-finished",
					  G_CALLBACK (on_git_list_branch_combo_command_finished),
					  branch_branch_combo);

	anjuta_command_start (ANJUTA_COMMAND (branch_list_command));
	
	g_signal_connect (G_OBJECT (dialog), "response", 
					  G_CALLBACK (on_create_branch_dialog_response), 
					  data);
	
	g_signal_connect (G_OBJECT (branch_revision_radio), "toggled",
					  G_CALLBACK (on_branch_revision_radio_toggled),
					  branch_revision_entry);

	g_signal_connect (G_OBJECT (branch_branch_radio), "toggled",
					  G_CALLBACK (on_branch_revision_radio_toggled),
					  branch_branch_combo);
	
	if (revision)
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (branch_revision_radio), 
									  TRUE);
		gtk_entry_set_text (GTK_ENTRY (branch_revision_entry), revision);
	}
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_create_branch (GtkAction *action, Git *plugin)
{
	create_branch_dialog (plugin, NULL);
}

void
on_log_menu_git_create_branch (GtkAction *action, Git *plugin)
{
	GitRevision *revision;
	gchar *sha;
	
	revision = git_log_get_selected_revision (plugin);
	
	if (revision)
	{
		sha = git_revision_get_sha (revision);
		
		create_branch_dialog (plugin, sha);
		g_free (sha);
	}
}
