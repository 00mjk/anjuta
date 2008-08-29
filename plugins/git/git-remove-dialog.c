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

#include "git-remove-dialog.h"

static void
on_remove_command_finished (AnjutaCommand *command, guint return_code,
							Git *plugin)
{
	AnjutaStatus *status;
	
	status = anjuta_shell_get_status (ANJUTA_PLUGIN (plugin)->shell,
									  NULL);
	
	anjuta_status (status, _("Git: File removed."), 5);
	
	git_report_errors (command, return_code);
	
	g_object_unref (command);
}


static void
on_remove_dialog_response (GtkDialog *dialog, gint response_id, 
						   GitUIData *data)
{
	GtkWidget *remove_file_chooser_button;
	GtkWidget *force_check;
	gchar *filename;
	const gchar *relative_filename;
	GitRemoveCommand *remove_command;
	
	if (response_id == GTK_RESPONSE_OK)
	{
		remove_file_chooser_button = glade_xml_get_widget (data->gxml, 
													"remove_file_chooser_button");
		force_check = glade_xml_get_widget (data->gxml, "force_check");
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (remove_file_chooser_button));
		
		if (git_check_input (GTK_WIDGET (dialog), remove_file_chooser_button, filename, 
							 _("Please select a file.")))
		{
			relative_filename = git_get_relative_path (filename,
												   data->plugin->project_root_directory);
			remove_command = git_remove_command_new_path (data->plugin->project_root_directory,
														  relative_filename,
														  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (force_check)));
			
			g_free (filename);
			
			
			g_signal_connect (G_OBJECT (remove_command), "command-finished",
							  G_CALLBACK (on_remove_command_finished),
							  data->plugin);
			
			anjuta_command_start (ANJUTA_COMMAND (remove_command));
		}
		else
			return;
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	git_ui_data_free (data);
}

static void
remove_dialog (Git *plugin, const gchar *filename)
{
	GladeXML *gxml;
	GtkWidget *dialog;
	GtkWidget *remove_file_chooser_button;
	GitUIData *data;
	
	gxml = glade_xml_new (GLADE_FILE, "remove_dialog", NULL);
	
	dialog = glade_xml_get_widget (gxml, "remove_dialog");
	remove_file_chooser_button = glade_xml_get_widget (gxml, 
													   "remove_file_chooser_button");
	if (filename)
	{
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (remove_file_chooser_button),
									   filename);
	}
	
	data = git_ui_data_new (plugin, gxml);
	
	g_signal_connect(G_OBJECT (dialog), "response", 
					 G_CALLBACK (on_remove_dialog_response), 
					 data);
	
	gtk_widget_show_all (dialog);
}

void
on_menu_git_remove (GtkAction *action, Git *plugin)
{
	remove_dialog (plugin, plugin->current_editor_filename);
}

void
on_fm_git_remove (GtkAction *action, Git *plugin)
{
	remove_dialog (plugin, plugin->current_fm_filename);
}
