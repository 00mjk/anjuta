 /*
  * mainmenu_callbacks.c
  * Copyright (C) 2000  Kh. Naba Kumar Singh
  * 
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  * 
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  * 
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sched.h>
#include <sys/wait.h>
#include <errno.h>

#include <gnome.h>

#include <libgnomeui/gnome-window-icon.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/resources.h>

#include "anjuta-app.h"
#include "help.h"
#include "about.h"
#include "action-callbacks.h"

void
on_exit1_activate (EggAction * action, GObject *app)
{
	if (on_anjuta_app_delete_event (app, NULL, NULL) == FALSE)
		on_anjuta_app_destroy_event (app, NULL);
}

void
on_set_preferences1_activate (EggAction * action, AnjutaApp *app)
{
	gtk_widget_show (GTK_WIDGET (app->preferences));
}

void
on_set_default_preferences1_activate (EggAction * action,
				      AnjutaApp *app)
{
	anjuta_preferences_reset_defaults (ANJUTA_PREFERENCES (app->preferences));
}

void
on_customize_shortcuts_activate(EggAction *action, AnjutaApp *app)
{
	gtk_widget_show (GTK_WIDGET (app->ui));
}

void
on_help_activate (EggAction *action, gpointer data)
{
	if (gnome_help_display ((const gchar*)data, NULL, NULL) == FALSE)
	{
	  anjuta_util_dialog_error (NULL, _("Unable to display help. Please make sure Anjuta documentation package is install. It can be downloaded from http://anjuta.org"));	
	}
}

void
on_gnome_pages1_activate (EggAction *action, AnjutaApp *app)
{
	if (anjuta_util_prog_is_installed ("devhelp", TRUE))
	{
		anjuta_res_help_search (NULL);
	}
}

void
on_context_help_activate (EggAction * action, AnjutaApp *app)
{
  /*	TextEditor* te;
	gboolean ret;
	gchar buffer[1000];
	
	te = anjuta_get_current_text_editor();
	if(!te) return;
	ret = aneditor_command (te->editor_id, ANE_GETCURRENTWORD, (long)buffer, (long)sizeof(buffer));
	if (ret == FALSE) return;
	anjuta_help_search(app->help_system, buffer);
  */
}

void
on_search_a_topic1_activate (EggAction * action, AnjutaApp *app)
{
  //anjuta_help_show (app->help_system);
}

void
on_url_activate (EggAction * action, gpointer user_data)
{
	if (user_data)
	{
		anjuta_res_url_show(user_data);
	}
}

void
on_about1_activate (EggAction * action, gpointer user_data)
{
	GtkWidget *about_dlg = about_box_new ();
	gtk_widget_show (about_dlg);
}
