/* 
 * mainmenu_callbacks.h Copyright (C) 2000 Kh. Naba Kumar Singh
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA */
#ifndef _MAINMENU_CALLBACKS_H_
#define _MAINMENU_CALLBACKS_H_

#include <gnome.h>
#include <libegg/menu/egg-toggle-action.h>

void on_exit1_activate (EggAction * action, GObject *app);
void on_set_preferences1_activate (EggAction * action, AnjutaApp *app);
void on_set_default_preferences1_activate (EggAction *action,
					   AnjutaApp *app);
void on_customize_shortcuts_activate (EggAction * action, AnjutaApp *app);
void on_layout_manager_activate(EggAction *action, AnjutaApp *app);

/*****************************************************************************/
void on_help_activate (EggAction * action, gpointer url);

void on_gnome_pages1_activate (EggAction * action, AnjutaApp *app);
void on_man_pages1_activate (EggAction * action, AnjutaApp *app);
void on_info_pages1_activate (EggAction * action, AnjutaApp *app);
void on_context_help_activate (EggAction * action, AnjutaApp *app);
void on_search_a_topic1_activate (EggAction * action, AnjutaApp *app);
void on_url_activate (EggAction * action, gpointer url);
void on_about1_activate (EggAction * action, gpointer user_data);

#endif
