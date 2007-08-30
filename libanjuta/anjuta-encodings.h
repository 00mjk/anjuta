/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * anjuta-encodings.h
 * Copyright (C) 2002 Paolo Maggi 
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02110-1301, USA.
 */
 
/*
 * Modified by the gedit Team, 2002. See the gedit AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the gedit ChangeLog files for a list of changes. 
 */

 /* Stolen from gedit - Naba */
 
#ifndef __ANJUTA_ENCODINGS_H__
#define __ANJUTA_ENCODINGS_H__

#include <glib.h>
#include <libanjuta/anjuta-preferences.h>
#include <glade/glade.h>

G_BEGIN_DECLS

typedef struct _AnjutaEncoding AnjutaEncoding;

const AnjutaEncoding* anjuta_encoding_get_from_charset (const gchar *charset);
const AnjutaEncoding* anjuta_encoding_get_from_index (gint index);

gchar* anjuta_encoding_to_string (const AnjutaEncoding* enc);
const gchar* anjuta_encoding_get_charset (const AnjutaEncoding* enc);

GList* anjuta_encoding_get_encodings (GList *encoding_strings);

void anjuta_encodings_init (AnjutaPreferences *pref, GladeXML *gxml);

/* Character encodings; preferences keys */
#define SAVE_ENCODING_ORIGINAL            "save.encoding.original"
#define SAVE_ENCODING_CURRENT_LOCALE      "save.encoding.current.locale"
#define SUPPORTED_ENCODINGS               "supported.encodings"

G_END_DECLS

#endif  /* __ANJUTA_ENCODINGS_H__ */
