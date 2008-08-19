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

#include "git-ignore-dialog.h"

static void
on_ignore_dialog_response (GtkDialog *dialog, gint response_id, 
						   GitUIData *data)
{
	GtkWidget *ignore_status_view;
	GList *selected_paths;
	GitIgnoreCommand *ignore_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{	
		ignore_status_view = glade_xml_get_widget (data->gxml, "ignore_status_view");
		selected_paths = anjuta_vcs_status_tree_view_get_selected (ANJUTA_VCS_STATUS_TREE_VIEW (ignore_status_view));
		ignore_command = git_ignore_command_new_list (data->plugin->project_root_directory,
													  selected_paths);
		
		git_command_free_path_list (selected_paths);
		
		g_signal_connect (G_OBJECT (ignore_command), "command-finished",
						  G_CALLBACK (on_git_command_finished),
						  data->plugin);
		
		anjuta_command_start (ANJUTA_COMMAND (ignore_command));
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
ignore_dialog (Git *plugin)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *ignore_select_all_button;
	GtkWidget *ignore_clear_button;
	GtkWidget *ignore_status_view;
	GtkWidget *ignore_status_progress_bar;
	GitStatusCommand *status_command;
	GitUIData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "ignore_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "ignore_dialog");
	ignore_select_all_button = glade_xml_get_widget (gxml, "ignore_select_all_button");
	ignore_clear_button = glade_xml_get_widget (gxml, "ignore_clear_button");
	ignore_status_view = glade_xml_get_widget (gxml, "ignore_status_view");
	ignore_status_progress_bar = glade_xml_get_widget (gxml, "ignore_status_progress_bar");
	
	status_command = git_status_command_new (plugin->project_root_directory,
											 GIT_STATUS_SECTION_UNTRACKED);
	
	g_signal_connect (G_OBJECT (ignore_select_all_button), "clicked",
					  G_CALLBACK (git_select_all_status_items),
					  ignore_status_view);
	
	g_signal_connect (G_OBJECT (ignore_clear_button), "clicked",
					  G_CALLBACK (git_clear_all_status_selections),
					  ignore_status_view);
	
	git_pulse_progress_bar (GTK_PROGRESS_BAR (ignore_status_progress_bar));
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (git_cancel_data_arrived_signal_disconnect),
					  ignore_status_view);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (git_hide_pulse_progress_bar),
					  ignore_status_progress_bar);
	
	g_signal_connect (G_OBJECT (status_command), "command-finished",
					  G_CALLBACK (on_git_command_finished),
					  NULL);
	
	g_signal_connect (G_OBJECT (status_command), "data-arrived",
					  G_CALLBACK (on_git_status_command_data_arrived),
					  ignore_status_view);
	
	g_object_weak_ref (G_OBJECT (ignore_status_view),
					   (GWeakNotify) git_disconnect_data_arrived_signals,
					   status_command);
	
	anjuta_command_start (ANJUTA_COMMAND (status_command));
	
	data = git_ui_data_new (plugin, gxml);
	
	g_signal_connect(G_OBJECT (dialog), "response", 
					 G_CALLBACK (on_ignore_dialog_response), 
					 data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_ignore (GtkAction *action, Git *plugin)
{
	ignore_dialog (plugin);
}
