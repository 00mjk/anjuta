/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.h
    Copyright (C) 2000 Naba Kumar, 2005 Johannes Schmid

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

#ifndef __FILE_WIZARD_PLUGIN__
#define __FILE_WIZARD_PLUGIN__

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-preferences.h>
#include <libanjuta/anjuta-ui.h>

typedef struct _AnjutaProjectImportPlugin AnjutaProjectImportPlugin;
typedef struct _AnjutaProjectImportPluginClass AnjutaProjectImportPluginClass;

struct _AnjutaProjectImportPlugin 
{
	AnjutaPlugin parent;
	AnjutaPreferences *prefs;
};

struct _AnjutaProjectImportPluginClass
{
	AnjutaPluginClass parent_class;
};

#endif
