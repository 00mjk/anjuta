/*
    toolbar.c
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

#include "toolbar_callbacks.h"

#include "resources.h"
#include "toolbar.h"
#include "anjuta.h"
#include "pixmaps.h"

GtkWidget *
create_main_toolbar (GtkWidget * anjuta_gui, MainToolbar * toolbar)
{
	GtkWidget *toolbar1;

	GtkWidget *toolbar_new;
	GtkWidget *toolbar_open;
	GtkWidget *toolbar_save;
	GtkWidget *toolbar_save_all;
	GtkWidget *toolbar_close;
	GtkWidget *toolbar_reload;
	GtkWidget *toolbar_undo;
	GtkWidget *toolbar_redo;
	GtkWidget *toolbar_led;
	GtkWidget *toolbar_print;
	GtkWidget *toolbar_detach;
	GtkWidget *toolbar_find;
	GtkWidget *toolbar_find_combo;
	GtkWidget *toolbar_find_entry;
	GtkWidget *toolbar_goto;
	GtkWidget *toolbar_line_entry;
	GtkWidget *toolbar_project;
	GtkWidget *toolbar_messages;
	GtkWidget *toolbar_help;

	GtkTooltips *tooltips;
	GtkWidget *tmp_toolbar_icon;
	gchar *filename;

	tooltips = gtk_tooltips_new ();

	toolbar1 =
		gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				 GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar1);
	gtk_widget_show (toolbar1);

	toolbar_led = gnome_animator_new_with_size (22, 22);

	filename = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_GREEN_LED);
	if (filename)
		gnome_animator_append_frame_from_file (GNOME_ANIMATOR
						       (toolbar_led),
						       filename, 3, 4, 200);
	if (filename)
		g_free (filename);

	filename = anjuta_res_get_pixmap_file (ANJUTA_PIXMAP_RED_LED);
	if (filename)
		gnome_animator_append_frame_from_file (GNOME_ANIMATOR
						       (toolbar_led),
						       filename, 3, 4, 200);
	if (filename)
		g_free (filename);

	gnome_animator_set_loop_type (GNOME_ANIMATOR (toolbar_led),
				      GNOME_ANIMATOR_LOOP_RESTART);
	gtk_widget_ref (toolbar_led);
	gtk_widget_show (toolbar_led);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), toolbar_led,
				   NULL, NULL);
	gtk_tooltips_set_tip (tooltips, toolbar_led,
			      _("Status LED: \n1) Red is busy." \
				"2)Green is ready.\n3) Blinking is ready, but busy in background."),
			      NULL);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_NEW_FILE, FALSE);
	toolbar_new =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("New"), _("New Text File"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_new);
	gtk_widget_show (toolbar_new);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_OPEN_FILE, FALSE);
	toolbar_open =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Open"), _("Open Text File"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_open);
	gtk_widget_show (toolbar_open);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_SAVE_FILE, FALSE);
	toolbar_save =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Save"), _("Save Current File"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_save);
	gtk_widget_show (toolbar_save);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_SAVE_ALL, FALSE);
	toolbar_save_all =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Save All"),
					    _
					    ("Save all currently opened Files, execpt Newfiles"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_save_all);
	gtk_widget_show (toolbar_save_all);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_CLOSE_FILE, FALSE);
	toolbar_close =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Close"),
					    _("Close Current File"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_close);
	gtk_widget_show (toolbar_close);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_RELOAD_FILE, FALSE);
	toolbar_reload =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Reload"),
					    _("Reload Current File"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_reload);
	gtk_widget_show (toolbar_reload);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_UNDO, FALSE);
	toolbar_undo =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Undo"),
					    _("Undo the last action"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_undo);
	gtk_widget_show (toolbar_undo);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_REDO, FALSE);
	toolbar_redo =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Redo"),
					    _("Redo the last udone action"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_redo);
	gtk_widget_show (toolbar_redo);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_UNDOCK, FALSE);
	toolbar_detach =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Detach"),
					    _("Detach the current page"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_detach);
	gtk_widget_show (toolbar_detach);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_PRINT, FALSE);
	toolbar_print =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Print"),
					    _("Print the current File"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_print);
	gtk_widget_show (toolbar_print);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_FIND, FALSE);
	toolbar_find =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Find"),
					    _("Search the given string"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_find);
	gtk_widget_show (toolbar_find);

	toolbar_find_combo = gtk_combo_new ();
	gtk_widget_ref (toolbar_find_combo);
	gtk_combo_set_case_sensitive (GTK_COMBO (toolbar_find_combo), TRUE);
	gtk_widget_show (toolbar_find_combo);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), toolbar_find_combo,
				   NULL, NULL);

	toolbar_find_entry = GTK_COMBO (toolbar_find_combo)->entry;
	gtk_widget_ref (toolbar_find_entry);
	gtk_widget_show (toolbar_find_entry);
	gtk_tooltips_set_tip (tooltips, toolbar_find_entry,
			      _
			      ("Enter the string to be searched in the current file"),
			      NULL);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_GOTO, FALSE);
	toolbar_goto =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Go To"),
					    _("Go to the given line number"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_goto);
	gtk_widget_show (toolbar_goto);

	toolbar_line_entry = gtk_entry_new ();
	gtk_widget_ref (toolbar_line_entry);
	gtk_widget_show (toolbar_line_entry);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), toolbar_line_entry,
				   _
				   ("Enter the line no. to go in the current file"),
				   NULL);
	gtk_widget_set_usize (toolbar_line_entry, 53, -2);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_PROJECT, FALSE);
	toolbar_project =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Project"),
					    _("Project Listing"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_project);
	gtk_widget_show (toolbar_project);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_MESSAGES, FALSE);
	toolbar_messages =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Messages"),
					    _("Compile/Build/Debug Messages"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_messages);
	gtk_widget_show (toolbar_messages);

	/* unimplemented */
	/* gtk_toolbar_append_space (GTK_TOOLBAR (toolbar1));*/

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_HELP, FALSE);
	toolbar_help =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Help"),
					    _("Context sensitive help"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_help);
	
	/* unimplemented */
	gtk_widget_hide (toolbar_help);

	gtk_signal_connect (GTK_OBJECT (toolbar_new), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_new_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_open), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_open_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_save), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_save_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_save_all), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_save_all_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_close), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_close_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_reload), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_reload_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_undo), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_undo_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_redo), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_redo_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_print), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_print_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_detach), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_detach_clicked),
			    NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar_find), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_find_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT
			    (GTK_COMBO (toolbar_find_combo)->entry),
			    "activate",
			    GTK_SIGNAL_FUNC (on_toolbar_find_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar_goto), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_goto_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_line_entry), "activate",
			    GTK_SIGNAL_FUNC (on_toolbar_goto_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar_project), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_project_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_messages), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_messages_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_help), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_help_clicked), NULL);

	toolbar->toolbar = toolbar1;

	toolbar->new = toolbar_new;
	toolbar->open = toolbar_open;
	toolbar->save = toolbar_save;
	toolbar->save_all = toolbar_save_all;
	toolbar->close = toolbar_close;
	toolbar->reload = toolbar_reload;
	toolbar->undo = toolbar_undo;
	toolbar->redo = toolbar_redo;
	toolbar->led = toolbar_led;
	toolbar->print = toolbar_print;
	toolbar->detach = toolbar_detach;
	toolbar->find = toolbar_find;
	toolbar->find_combo = toolbar_find_combo;
	toolbar->find_entry = toolbar_find_entry;
	toolbar->go_to = toolbar_goto;
	toolbar->line_entry = toolbar_line_entry;
	toolbar->project = toolbar_project;
	toolbar->messages = toolbar_messages;
	toolbar->help = toolbar_help;

	return toolbar1;
}

GtkWidget *
create_extended_toolbar (GtkWidget * anjuta_gui, ExtendedToolbar * toolbar)
{
	GtkWidget *toolbar2;

	GtkWidget *toolbar_open_project;
	GtkWidget *toolbar_save_project;
	GtkWidget *toolbar_close_project;

	GtkWidget *toolbar_compile;
	GtkWidget *toolbar_configure;
	GtkWidget *toolbar_build;
	GtkWidget *toolbar_build_all;
	GtkWidget *toolbar_exec;
	GtkWidget *toolbar_debug;
	GtkWidget *toolbar_stop;
	GtkWidget *tmp_toolbar_icon;

	toolbar2 =
		gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				 GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar2);
	gtk_widget_show (toolbar2);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_OPEN_PROJECT, FALSE);
	toolbar_open_project =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Open Project"),
					    _("Open a Project"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_open_project);
	gtk_widget_show (toolbar_open_project);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_SAVE_PROJECT, FALSE);
	toolbar_save_project =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Save Project"),
					    _("Save the current Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_save_project);
	gtk_widget_show (toolbar_save_project);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_CLOSE_PROJECT,
			       FALSE);
	toolbar_close_project =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Close Project"),
					    _("Close the current Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_close_project);
	gtk_widget_show (toolbar_close_project);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_COMPILE, FALSE);
	toolbar_compile =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Compile"),
					    _("Compile the current file"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_compile);
	gtk_widget_show (toolbar_compile);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_CONFIGURE, FALSE);
	toolbar_configure =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Configure"),
					    _("Run Configure"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_configure);
	gtk_widget_show (toolbar_configure);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_BUILD, FALSE);
	toolbar_build =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Build"),
					    _
					    ("Build current File or Build the source directory of the Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_build);
	gtk_widget_show (toolbar_build);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_BUILD_ALL, FALSE);
	toolbar_build_all =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Build_all"),
					    _
					    ("Build from the top directory of the Project"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_build_all);
	gtk_widget_show (toolbar_build_all);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_EXECUTE, FALSE);
	toolbar_exec =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Execute"),
					    _("Execute the Program"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_exec);
	gtk_widget_show (toolbar_exec);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_DEBUG, FALSE);
	toolbar_debug =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Debug"),
					    _("Start the Debugger"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_debug);
	gtk_widget_show (toolbar_debug);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_BUILD_STOP, FALSE);
	toolbar_stop =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Stop"),
					    _("Stop Compile or Build"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_stop);
	gtk_widget_show (toolbar_stop);

	gtk_signal_connect (GTK_OBJECT (toolbar_open_project), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_open_project_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_save_project), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_save_project_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_close_project), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_toolbar_close_project_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar_compile), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_compile_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_configure), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_configure_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_build), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_build_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_build_all), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_build_all_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_exec), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_exec_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_debug), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_debug_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_stop), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_stop_clicked), NULL);

	toolbar->toolbar = toolbar2;
	toolbar->open_project = toolbar_open_project;
	toolbar->save_project = toolbar_save_project;
	toolbar->close_project = toolbar_close_project;

	toolbar->compile = toolbar_compile;
	toolbar->configure = toolbar_configure;
	toolbar->build = toolbar_build;
	toolbar->build_all = toolbar_build_all;
	toolbar->exec = toolbar_exec;
	toolbar->debug = toolbar_debug;
	toolbar->stop = toolbar_stop;
	return toolbar2;
}

GtkWidget *
create_tags_toolbar (GtkWidget * anjuta_gui, TagsToolbar * toolbar)
{
	GtkWidget *window1;
	GtkWidget *toolbar1;
	GtkWidget *optionmenu1;
	GtkWidget *optionmenu1_menu;
	GtkWidget *combo1;
	GtkWidget *combo_entry1;
	GtkWidget *combo_list1;
	GtkWidget *label1;
	GtkWidget *label2;
	GtkWidget *combo2;
	GtkWidget *combo_entry2;
	GtkWidget *combo_list2;
	GtkWidget *tmp_toolbar_icon;
	GtkWidget *button1;
	GtkTooltips *tooltips;

	tooltips = gtk_tooltips_new ();

	window1 = anjuta_gui;

	toolbar1 =
		gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				 GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar1);
	gtk_widget_show (toolbar1);

	optionmenu1 = gtk_option_menu_new ();
	gtk_widget_ref (optionmenu1);
	gtk_widget_show (optionmenu1);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), optionmenu1,
				   _("Tag Type"), NULL);
	optionmenu1_menu = create_tag_menu ();

	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1),
				  optionmenu1_menu);

	label1 = gtk_label_new (_("File:"));
	gtk_widget_ref (label1);
	gtk_widget_show (label1);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), label1, NULL,
				   NULL);
	/* gtk_widget_set_usize (label1, 50, -2); */
	gtk_misc_set_padding (GTK_MISC (label1), 5, 0);

	combo1 = gtk_combo_new ();
	gtk_widget_ref (combo1);
	gtk_widget_show (combo1);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), combo1, NULL,
				   NULL);
	gtk_widget_set_usize (combo1, 200, -2);

	combo_entry1 = GTK_COMBO (combo1)->entry;
	gtk_widget_ref (combo_entry1);
	gtk_widget_show (combo_entry1);
	gtk_tooltips_set_tip (tooltips, combo_entry1, _("Source File"), NULL);
	gtk_entry_set_editable (GTK_ENTRY (combo_entry1), FALSE);

	combo_list1 = GTK_COMBO (combo1)->list;

	label2 = gtk_label_new (_("Function:"));
	gtk_widget_ref (label2);
	gtk_widget_show (label2);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), label2, NULL,
				   NULL);
	/* gtk_widget_set_usize (label2, 90, -2);*/
	gtk_misc_set_padding (GTK_MISC (label2), 5, 0);

	combo2 = gtk_combo_new ();
	gtk_widget_ref (combo2);
	gtk_widget_show (combo2);
	gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), combo2, NULL,
				   NULL);
	gtk_widget_set_usize (combo2, 245, -2);

	combo_entry2 = GTK_COMBO (combo2)->entry;
	gtk_widget_ref (combo_entry2);
	gtk_widget_show (combo_entry2);
	gtk_tooltips_set_tip (tooltips, combo_entry2, _("Tag in the file"),
			      NULL);
	gtk_entry_set_editable (GTK_ENTRY (combo_entry2), FALSE);

	combo_list2 = GTK_COMBO (combo2)->list;

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_WIZARD, FALSE);
	button1 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("wizard"), _("The Wizard"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button1);
	
	/* Unimplemented */
	gtk_widget_hide (button1);


	gtk_signal_connect (GTK_OBJECT (combo_entry1), "changed",
			    GTK_SIGNAL_FUNC (on_tag_combo_entry_changed),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (combo_entry2), "changed",
			    GTK_SIGNAL_FUNC (on_member_combo_entry_changed),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (combo_list1), "select_child",
			    GTK_SIGNAL_FUNC (on_tag_combo_list_select_child),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (combo_list2), "select_child",
			    GTK_SIGNAL_FUNC
			    (on_member_combo_list_select_child), NULL);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked",
			    GTK_SIGNAL_FUNC (on_browser_wizard_clicked),
			    NULL);


	toolbar->toolbar = toolbar1;
	toolbar->tags_type = optionmenu1;
	toolbar->tags_menu = optionmenu1_menu;
	toolbar->tag_label = label1;
	toolbar->tag_combo = combo1;
	toolbar->tag_entry = combo_entry1;
	toolbar->member_label = label2;
	toolbar->member_combo = combo2;
	toolbar->member_entry = combo_entry2;
	toolbar->wizard = button1;

	return toolbar1;
}

GtkWidget *
create_browser_toolbar (GtkWidget * anjuta_gui, BrowserToolbar * toolbar)
{
	GtkWidget *window1;
	GtkWidget *toolbar2;

	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button4;
	GtkWidget *button5;
	GtkWidget *button6;
	GtkWidget *button7;
	GtkWidget *button8;
	GtkWidget *button9;
	GtkWidget *button10;
	GtkTooltips *tooltips;
	GtkWidget *tmp_toolbar_icon;

	tooltips = gtk_tooltips_new ();

	window1 = anjuta_gui;

	toolbar2 =
		gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				 GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar2);
	gtk_widget_show (toolbar2);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar2),
				     GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2),
				       GTK_RELIEF_NONE);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_BOOKMARK_TOGGLE, FALSE);
	button2 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Toggle"),
					    _
					    ("Toggle bookmark at current location"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button2);
	gtk_widget_show (button2);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_BOOKMARK_FIRST, FALSE);
	button3 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("First"),
					    _
					    ("Goto first bookmark in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button3);
	gtk_widget_show (button3);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_BOOKMARK_PREV, FALSE);
	button4 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Prev"),
					    _
					    ("Goto previous bookmark in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button4);
	gtk_widget_show (button4);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_BOOKMARK_NEXT, FALSE);
	button5 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Next"),
					    _
					    ("Goto next bookmark in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button5);
	gtk_widget_show (button5);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_BOOKMARK_LAST, FALSE);
	button6 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Last"),
					    _
					    ("Goto last bookmark in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button6);
	gtk_widget_show (button6);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_ERROR_PREV, FALSE);
	button7 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Prev"),
					    _
					    ("Goto previous message in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button7);
	gtk_widget_show (button7);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_ERROR_NEXT, FALSE);
	button8 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Next"),
					    _
					    ("Goto next message in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button8);
	gtk_widget_show (button8);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_BLOCK_START, FALSE);
	button9 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Start"),
					    _
					    ("Goto start of current code block"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button9);
	gtk_widget_show (button9);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_BLOCK_END, FALSE);
	button10 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("End"),
					    _
					    ("Goto end of current code block"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button10);
	gtk_widget_show (button10);

	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_browser_toggle_bookmark_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_browser_first_bookmark_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_browser_prev_bookmark_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (button5), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_browser_next_bookmark_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (button6), "clicked",
			    GTK_SIGNAL_FUNC
			    (on_browser_last_bookmark_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (button7), "clicked",
			    GTK_SIGNAL_FUNC (on_browser_prev_mesg_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button8), "clicked",
			    GTK_SIGNAL_FUNC (on_browser_next_mesg_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button9), "clicked",
			    GTK_SIGNAL_FUNC (on_browser_block_start_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button10), "clicked",
			    GTK_SIGNAL_FUNC (on_browser_block_end_clicked),
			    NULL);

	toolbar->toolbar = toolbar2;
	toolbar->toggle_bookmark = button2;
	toolbar->first_bookmark = button3;
	toolbar->prev_bookmark = button4;
	toolbar->next_bookmark = button5;
	toolbar->last_bookmark = button6;
	toolbar->prev_error = button7;
	toolbar->next_error = button8;
	toolbar->block_start = button9;
	toolbar->block_end = button10;

	return toolbar2;
}

GtkWidget *
create_debug_toolbar (GtkWidget * anjuta_gui, DebugToolbar * toolbar)
{
	GtkWidget *toolbar3;
	GtkWidget *toolbar_go;
	GtkWidget *toolbar_run_to_cursor;
	GtkWidget *toolbar_step_in;
	GtkWidget *toolbar_step_out;
	GtkWidget *toolbar_step_over;
	GtkWidget *toolbar_toggle_bp;
	GtkWidget *toolbar_interrupt;
	GtkWidget *toolbar_frame;
	GtkWidget *toolbar_watch;
	GtkWidget *toolbar_inspect;
	GtkWidget *toolbar_stack;
	GtkWidget *toolbar_registers;
	GtkWidget *toolbar_stop;
	GtkWidget *tmp_toolbar_icon;

	toolbar3 =
		gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				 GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar3);
	gtk_widget_show (toolbar3);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_CONTINUE, FALSE);
	toolbar_go =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Go"),
					    _("Go or continue execution"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_go);
	gtk_widget_show (toolbar_go);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_RUN_TO_CURSOR, FALSE);
	toolbar_run_to_cursor =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Run"),
					    _("Run to cursor"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_run_to_cursor);
	gtk_widget_show (toolbar_run_to_cursor);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_STEP_IN, FALSE);
	toolbar_step_in =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Step in"),
					    _("Single step in execution"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_step_in);
	gtk_widget_show (toolbar_step_in);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_STEP_OVER, FALSE);
	toolbar_step_over =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Step Over"),
					    _("step over the function"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_step_over);
	gtk_widget_show (toolbar_step_over);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_STEP_OUT, FALSE);
	toolbar_step_out =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Step Out"),
					    _("step out of the function"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_step_out);
	gtk_widget_show (toolbar_step_out);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar3));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_INTERRUPT, FALSE);
	toolbar_interrupt =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Interrupt"),
					    _
					    ("Interrupt the Program execution"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_interrupt);
	gtk_widget_show (toolbar_interrupt);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar3));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_BREAKPOINT, FALSE);
	toolbar_toggle_bp =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Breakpoint"),
					    _("Toggle breakpoint at cursor"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (toolbar_toggle_bp);
	gtk_widget_show (toolbar_toggle_bp);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_INSPECT, FALSE);
	toolbar_inspect =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Inspect"),
					    _
					    ("Inspect or evaluate an expression"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_inspect);
	gtk_widget_show (toolbar_inspect);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_FRAME, FALSE);
	toolbar_frame =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Frame"),
					    _
					    ("Display the current frame information"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_frame);
	gtk_widget_show (toolbar_frame);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar3));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_WATCH, FALSE);
	toolbar_watch =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Watch"),
					    _
					    ("Watch expresions during execution"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_watch);
	gtk_widget_show (toolbar_watch);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_STACK, FALSE);
	toolbar_stack =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Stack"),
					    _("Stack trace of the program"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_stack);
	gtk_widget_show (toolbar_stack);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_REGISTERS, FALSE);
	toolbar_registers =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Registers"),
					    _
					    ("CPU registers and their contents"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_registers);
	gtk_widget_show (toolbar_registers);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar3));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (anjuta_gui, ANJUTA_PIXMAP_DEBUG_STOP, FALSE);
	toolbar_stop =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar3),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Stop"),
					    _("End the debugging session"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (toolbar_stop);
	gtk_widget_show (toolbar_stop);

	gtk_signal_connect (GTK_OBJECT (toolbar_run_to_cursor), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_run_to_cursor_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar_step_in), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_step_in_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_step_out), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_step_out_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_step_over), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_step_over_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_toggle_bp), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_toggle_bp_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_go), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_go_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (toolbar_watch), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_watch_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_stack), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_stack_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_frame), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_frame_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_registers), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_registers_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_inspect), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_inspect_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_interrupt), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_interrupt_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (toolbar_stop), "clicked",
			    GTK_SIGNAL_FUNC (on_toolbar_debug_stop_clicked), NULL);

	toolbar->toolbar = toolbar3;
	toolbar->go = toolbar_go;
	toolbar->run_to_cursor = toolbar_run_to_cursor;
	toolbar->step_in = toolbar_step_in;
	toolbar->step_out = toolbar_step_out;
	toolbar->step_over = toolbar_step_over;
	toolbar->toggle_bp = toolbar_toggle_bp;
	toolbar->watch = toolbar_watch;
	toolbar->frame = toolbar_frame;
	toolbar->interrupt = toolbar_interrupt;
	toolbar->stack = toolbar_stack;
	toolbar->inspect = toolbar_inspect;
	toolbar->stop = toolbar_stop;
	return toolbar3;
}



GtkWidget *
create_format_toolbar (GtkWidget * anjuta_gui, FormatToolbar * toolbar)
{
	GtkWidget *window1;
	GtkWidget *toolbar2;

	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *button4;
	GtkWidget *button5;
	GtkWidget *button6;
	GtkWidget *button7;
	GtkWidget *button8;
	GtkWidget *button9;
	GtkWidget *button10;
	GtkWidget *button11;
	GtkTooltips *tooltips;
	GtkWidget *tmp_toolbar_icon;

	tooltips = gtk_tooltips_new ();

	window1 = anjuta_gui;

	toolbar2 =
		gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL,
				 GTK_TOOLBAR_ICONS);
	gtk_widget_ref (toolbar2);
	gtk_widget_show (toolbar2);
	gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar2),
				     GTK_TOOLBAR_SPACE_LINE);
	gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2),
				       GTK_RELIEF_NONE);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_FOLD_TOGGLE, FALSE);
	button2 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Toggle"),
					    _
					    ("Toggle current code fold hide/show"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button2);
	gtk_widget_show (button2);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_FOLD_CLOSE, FALSE);
	button4 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Close all"),
					    _
					    ("Close all code folds in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button4);
	gtk_widget_show (button4);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_FOLD_OPEN, FALSE);
	button3 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Open all"),
					    _
					    ("Open all code folds in this document"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button3);
	gtk_widget_show (button3);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_BLOCK_SELECT, FALSE);
	button5 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Select"),
					    _("Select current code block"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button5);
	gtk_widget_show (button5);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_INDENT_INC, FALSE);
	button6 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Indent inc"),
					    _
					    ("Increase indentation of block/line"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button6);
	gtk_widget_show (button6);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_INDENT_DCR, FALSE);
	button7 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Indent dcr"),
					    _
					    ("Decrease indentation of block/line"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button7);
	gtk_widget_show (button7);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_INDENT_AUTO, FALSE);
	button8 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Autoformat"),
					    _
					    ("Automatically format the codes"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button8);
	gtk_widget_show (button8);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_AUTOFORMAT_SETTING, FALSE);
	button9 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Format style"),
					    _("Select autoformat style"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button9);
	gtk_widget_show (button9);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar2));

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1, ANJUTA_PIXMAP_CALLTIP, FALSE);
	button10 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Calltip"), _("Show Calltip"),
					    NULL, tmp_toolbar_icon, NULL,
					    NULL);
	gtk_widget_ref (button10);
	
	/* unimplemented */
	 gtk_widget_hide (button10);

	tmp_toolbar_icon =
		anjuta_res_get_pixmap_widget (window1,
					   ANJUTA_PIXMAP_AUTOCOMPLETE, FALSE);
	button11 =
		gtk_toolbar_append_element (GTK_TOOLBAR (toolbar2),
					    GTK_TOOLBAR_CHILD_BUTTON, NULL,
					    _("Autocomplete"),
					    _("Autocomplete word"), NULL,
					    tmp_toolbar_icon, NULL, NULL);
	gtk_widget_ref (button11);
	gtk_widget_show (button11);

	gtk_signal_connect (GTK_OBJECT (button2), "clicked",
			    GTK_SIGNAL_FUNC (on_format_fold_toggle_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button3), "clicked",
			    GTK_SIGNAL_FUNC (on_format_fold_open_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button4), "clicked",
			    GTK_SIGNAL_FUNC (on_format_fold_close_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button5), "clicked",
			    GTK_SIGNAL_FUNC (on_format_block_select_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button6), "clicked",
			    GTK_SIGNAL_FUNC (on_format_indent_inc_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button7), "clicked",
			    GTK_SIGNAL_FUNC (on_format_indent_dcr_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button8), "clicked",
			    GTK_SIGNAL_FUNC (on_format_indent_auto_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button9), "clicked",
			    GTK_SIGNAL_FUNC (on_format_indent_style_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button10), "clicked",
			    GTK_SIGNAL_FUNC (on_format_calltip_clicked),
			    NULL);
	gtk_signal_connect (GTK_OBJECT (button11), "clicked",
			    GTK_SIGNAL_FUNC (on_format_autocomplete_clicked),
			    NULL);

	toolbar->toolbar = toolbar2;
	toolbar->toggle_fold = button2;
	toolbar->open_all_folds = button3;
	toolbar->close_all_folds = button4;
	toolbar->block_select = button5;
	toolbar->indent_increase = button6;
	toolbar->indent_decrease = button7;
	toolbar->autoformat = button8;
	toolbar->set_autoformat_style = button9;
	toolbar->calltip = button10;
	toolbar->autocomplete = button11;

	return toolbar2;
}

static GnomeUIInfo menu1_uiinfo[] = {
	{
	 GNOME_APP_UI_ITEM, N_("Functions"),
	 NULL,
	 on_tag_functions_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Classes"),
	 NULL,
	 on_tag_classes_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Structs"),
	 NULL,
	 on_tag_structs_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Unions"),
	 NULL,
	 on_tag_unions_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Enums"),
	 NULL,
	 on_tag_enums_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Variables"),
	 NULL,
	 on_tag_variables_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_SEPARATOR,
	{
	 GNOME_APP_UI_ITEM, N_("Macros"),
	 NULL,
	 on_tag_macros_activate, NULL, NULL,
	 GNOME_APP_PIXMAP_NONE, NULL,
	 0, 0, NULL},
	GNOMEUIINFO_END
};

GtkWidget *
create_tag_menu ()
{
	GtkWidget *menu1;

	menu1 = gtk_menu_new ();
	gnome_app_fill_menu (GTK_MENU_SHELL (menu1), menu1_uiinfo,
			     NULL, FALSE, 0);
	return menu1;
}
