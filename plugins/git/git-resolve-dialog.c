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

#include "git-resolve-dialog.h"

static void
on_add_command_finished (AnjutaCommand *command, guint return_code,
						 Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: Resolve complete."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_resolve_dialog_response (GtkDialog *dialog, gint response_id, 
							GitUIData *data)
{
	GtkWidget *resolve_status_view;
	GList *selected_paths;
	GitAddCommand *add_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		resolve_status_view = glade_xml_get_widget (data->gxml, "resolve_status_view");
		selected_paths = anjuta_vcs_status_tree_view_get_selected (ANJUTA_VCS_STATUS_TREE_VIEW (resolve_status_view));
		add_command = git_add_command_new_list (data->plugin->project_root_directory,
												selected_paths, FALSE);
		
		git_command_free_path_list (selected_paths);
		
		g_signal_connect (G_OBJECT (add_command), "command-finished",
						  G_CALLBACK (on_add_command_finished),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (add_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
resolve_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *resolve_select_all_button;
	GtkWidget *resolve_clear_button;
	GtkWidget *resolve_status_view;
	GtkWidget *resolve_status_progress_bar;
	GitStatusCommand *status_command;
	GitUIData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "resolve_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "resolve_dialog");
	resolve_select_all_button = glade_xml_get_widget (gxml, "resolve_select_all_button");
	resolve_clear_button = glade_xml_get_widget (gxml, "resolve_clear_button");
	resolve_status_view = glade_xml_get_widget (gxml, "resolve_status_view");
	resolve_status_progress_bar = glade_xml_get_widget (gxml, "resolve_status_progress_bar");
	
	status_command = git_status_command_new (plugin->project_root_directory,
											 GIT_STATUS_SECTION_NOT_UPDATED);
	
	g_signal_connect (G_OBJECT (resolve_select_all_button), "clicked",
					  G_CALLBACK (git_select_all_status_items),
					  resolve_status_view);
	
	g_signal_connect (G_OBJECT (resolve_clear_button), "clicked",
					  G_CALLBACK (git_clear_all_status_selections),
					  resolve_status_view);
	
	git_pulse_progress_bar (GTK_PROGRESS_BAR (resolve_status_progress_bar));
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (git_cancel_data_arrived_signal_disconnect),
					  resolve_status_view);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (git_hide_pulse_progress_bar),
					  resolve_status_progress_bar);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (on_git_command_finished),
					  NULL);
	
	g_signal_connect (G_OBJECT (status_command), "data-arrived",
					  G_CALLBACK (on_git_status_command_data_arrived),
					  resolve_status_view);
	
	g_object_weak_ref (G_OBJECT (resolve_status_view),
					   (GWeakNotify) git_disconnect_data_arrived_signals,
					   status_command);
	
	anjuta_command_start (ANJUTA_COMMAND (status_command));
	
	data = git_ui_data_new (plugin, gxml);
	
	g_signal_connect(G_OBJECT (dialog), "response", 
					 G_CALLBACK (on_resolve_dialog_response), 
					 data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_resolve (GtkAction *action, Git *plugin)
{
	resolve_dialog (plugin);
}
