/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    autogen.h
    Copyright (C) 2004 Sebastien Granjoux

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

#ifndef __AUTOGEN_H__
#define __AUTOGEN_H__

#include "property.h"
#include <libanjuta/anjuta-preferences.h>
#include <glib.h>

typedef struct _NPWAutogen NPWAutogen;
typedef void (*NPWAutogenFunc) (NPWAutogen* autogen, gpointer data);
typedef void (*NPWAutogenOutputFunc) (const gchar* output, gpointer data);

NPWAutogen* npw_autogen_new(void);
void npw_autogen_destroy(NPWAutogen* this);

gboolean npw_autogen_add_default_definition(NPWAutogen* this, AnjutaPreferences* pref);
gboolean npw_autogen_add_definition(NPWAutogen* this, NPWPage* page);
gboolean npw_autogen_remove_definition(NPWAutogen* this, NPWPage* page);
gboolean npw_autogen_write_definition_file(NPWAutogen* this);

gboolean npw_autogen_set_input_file(NPWAutogen* this, const gchar* filename, const gchar* start_macro, const gchar* end_macro);
gboolean npw_autogen_set_output_file(NPWAutogen* this, const gchar* filename);
gboolean npw_autogen_set_output_callback(NPWAutogen* this, NPWAutogenOutputFunc func, gpointer data);

gboolean npw_autogen_execute(NPWAutogen* this, NPWAutogenFunc func, gpointer data);

#endif
