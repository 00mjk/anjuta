/* 
    toolbar_callbacks.h
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
#include <gnome.h>

void
on_toolbar_new_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_open_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_save_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_save_all_clicked                (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_close_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_reload_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_undo_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_redo_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_print_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_detach_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_find_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_goto_clicked                (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_project_clicked                (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_messages_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_help_clicked                (GtkButton       *button,
                                        gpointer         user_data);


void
on_toolbar_class_entry_changed         (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_toolbar_function_entry_changed      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_toolbar_open_project_clicked                (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_save_project_clicked                (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_close_project_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_compile_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_configure_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_build_clicked               (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_build_all_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_exec_clicked               (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_debug_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_stop_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_go_clicked                  (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_run_to_cursor_clicked (GtkButton * button, gpointer user_data);

void
on_toolbar_step_in_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_step_out_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_step_over_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_toolbar_toggle_bp_clicked           (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_watch_clicked           (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_stack_clicked           (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_registers_clicked           (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_frame_clicked           (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_inspect_clicked           (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_interrupt_clicked           (GtkButton       *button,
                                        gpointer         user_data);
void
on_toolbar_debug_stop_clicked           (GtkButton       *button,
                                        gpointer         user_data);
void
on_tag_combo_entry_changed             (GtkEditable     *editable,
                                        gpointer         user_data);
void
on_member_combo_entry_changed             (GtkEditable     *editable,
                                        gpointer         user_data);


void
on_tag_combo_list_select_child                  (GtkList         *list,
                                        GtkWidget       *widget,
                                        gpointer         user_data);

void
on_member_combo_list_select_child                  (GtkList         *list,
                                        GtkWidget       *widget,
                                        gpointer         user_data);
void
on_browser_widzard_clicked  (GtkButton       *button,
                                        gpointer         user_data);
void
on_browser_toggle_bookmark_clicked  (GtkButton       *button,
                                        gpointer         user_data);
void
on_browser_first_bookmark_clicked  (GtkButton       *button,
                                        gpointer         user_data);
void
on_browser_prev_bookmark_clicked  (GtkButton       *button,
                                        gpointer         user_data);
void
on_browser_next_bookmark_clicked  (GtkButton       *button,
                                        gpointer         user_data);
void
on_browser_last_bookmark_clicked  (GtkButton       *button,
                                        gpointer         user_data);
void
on_browser_prev_mesg_clicked  (GtkButton       *button,
                                        gpointer         user_data);
void
on_browser_next_mesg_clicked  (GtkButton       *button,
                                        gpointer         user_data);
void
on_browser_block_start_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_browser_block_end_clicked   (GtkButton       *button,
                  gpointer         user_data);

void
on_format_fold_toggle_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_fold_open_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_fold_close_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_indent_inc_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_indent_dcr_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_indent_auto_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_indent_style_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_block_select_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_calltip_clicked   (GtkButton       *button,
                  gpointer         user_data);
void
on_format_autocomplete_clicked   (GtkButton       *button,
                  gpointer         user_data);

void
on_tag_functions_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tag_classes_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tag_structs_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tag_unions_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tag_enums_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tag_variables_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tag_macros_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tag_browser_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
