/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*  plugin.h (c) Johannes Schmid 2004
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
 
#ifndef _MV_PLUGIN_H
#define _MV_PLUGIN_H

#include <libanjuta/anjuta-plugin.h>

typedef struct _MessageViewPlugin MessageViewPlugin;
typedef struct _MessageViewPluginClass MessageViewPluginClass;

struct _MessageViewPlugin {
	AnjutaPlugin parent;
	GtkWidget* msgman;
	
	gint uiid;
};

struct _MessageViewPluginClass {
	AnjutaPluginClass parent_class;
};

#endif /* _MV_PLUGIN_H */
