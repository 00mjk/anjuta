/*  cvs_gui.h (c) Johannes Schmid 2002
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "cvs_gui.h"

#include <gnome.h>

#ifndef CVS_CBS_H
#define CVS_CBS_H

/* CVS Settings */
gboolean on_cvs_settings_apply (GtkWidget * button, CVSSettingsGUI * gui);
void on_cvs_settings_ok (GtkWidget * button, CVSSettingsGUI * gui);
void on_cvs_settings_cancel (GtkWidget * button, CVSSettingsGUI * gui);

void on_combo_server_type_changed (GtkWidget * entry, CVSSettingsGUI * gui);
void on_entry_changed (GtkWidget * entry, CVSSettingsGUI * gui);

/* CVS File Dialog */
void on_cvs_ok (GtkWidget * button, CVSFileGUI * gui);
void on_cvs_cancel (GtkWidget * button, CVSFileGUI* gui);

/* CVS File Diff Dialog */
void on_cvs_diff_ok (GtkWidget* button, CVSFileDiffGUI * gui);
void on_cvs_diff_cancel (GtkWidget* button, CVSFileDiffGUI * gui);

#endif
