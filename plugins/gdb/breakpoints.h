/*
    breakpoints.h
    Copyright (C) 2000  Naba Kumar <naba@gnome.org>

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

#ifndef _BREAKPOINTS_DBASE_H_
#define _BREAKPOINTS_DBASE_H_

#include <glade/glade.h>
#include <stdio.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include "debugger.h"

G_BEGIN_DECLS

typedef struct _BreakpointsDBase BreakpointsDBase;
typedef struct _BreakpointsDBasePriv BreakpointsDBasePriv;

struct _BreakpointsDBase
{
	BreakpointsDBasePriv *priv;
};

BreakpointsDBase *breakpoints_dbase_new (AnjutaPlugin *plugin,
										 Debugger *debugger);

void breakpoints_dbase_show (BreakpointsDBase * bd);

void breakpoints_dbase_hide (BreakpointsDBase * bd);

void breakpoints_dbase_clear (BreakpointsDBase * bd);

void breakpoints_dbase_destroy (BreakpointsDBase * bd);

void breakpoints_dbase_add (BreakpointsDBase *bd);

gboolean breakpoints_dbase_toggle_breakpoint (BreakpointsDBase* bd, guint l);

gboolean breakpoints_dbase_toggle_doubleclick (BreakpointsDBase* bd,
											   guint line);

void breakpoints_dbase_toggle_singleclick (BreakpointsDBase* bd,
										   guint line);

void breakpoints_dbase_disable_all (BreakpointsDBase *bd);
void breakpoints_dbase_enable_all (BreakpointsDBase *bd);
void breakpoints_dbase_remove_all (BreakpointsDBase *bd);

void breakpoints_dbase_set_all (BreakpointsDBase * bd);

void breakpoints_dbase_update_controls (BreakpointsDBase * bd);

void breakpoints_dbase_set_all_in_editor (BreakpointsDBase* bd,
										  IAnjutaEditor* te);
void breakpoints_dbase_clear_all_in_editor (BreakpointsDBase* bd,
											GObject* dead_te);
/* TODO
gboolean breakpoints_dbase_save_yourself (BreakpointsDBase * bd, FILE * stream);
gboolean breakpoints_dbase_load_yourself (BreakpointsDBase * bd, PropsID props);
void breakpoints_dbase_save (BreakpointsDBase * bd, ProjectDBase * pdb );
void breakpoints_dbase_load (BreakpointsDBase * bd, ProjectDBase *p );
*/

G_END_DECLS
											
#endif
