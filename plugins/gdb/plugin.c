/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    plugin.c
    Copyright (C) 2004 Naba Kumar

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

#include <config.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-debug.h>

#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-debugger.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
// #include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-message-manager.h>
#include <libanjuta/interfaces/ianjuta-message-view.h>

#include "plugin.h"
#include "debugger.h"
#include "actions.h"

#define ICON_FILE "anjuta-gdb.plugin.png"
#define UI_FILE PACKAGE_DATA_DIR"/ui/anjuta-gdb-plugin.ui"

static gpointer parent_class;

static const gchar * MESSAGE_VIEW_TITLE = N_("Debug");

/* GDB actions */

static GtkActionEntry actions_gdb[] =
{
	{
		"ActionMenuDebug",                        /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Debug"),                             /* Display label */
		NULL,                                     /* short-cut */
		NULL,                                     /* Tooltip */
		NULL                                      /* action callback */
	},
	{
		"ActionGdbSetBreakpoint",                 /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Set Breakpoint ..."),                 /* Display label */
		NULL,                                     /* short-cut */
		N_("Set a breakpoint"),                   /* Tooltip */
		G_CALLBACK (on_set_breakpoint1_activate)  /* action callback */
	},
	{
		"ActionGdbBreakpoints",                   /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("_Breakpoints ..."),                   /* Display label */
		NULL,                                     /* short-cut */
		N_("Edit breakpoints"),                   /* Tooltip */
		G_CALLBACK (on_show_breakpoints1_activate)/* action callback */
	},
	{
		"ActionGdbDisableAllBreakpoints",         /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("Disable all Breakpoints"),            /* Display label */
		NULL,                                     /* short-cut */
		N_("Deactivate all breakpoints"),         /* Tooltip */
		G_CALLBACK (on_disable_all_breakpoints1_activate)/* action callback */
	},
	{
		"ActionGdbClearAllBreakpoints",           /* Action name */
		NULL,                                     /* Stock icon, if any */
		N_("C_lear all Breakpoints"),             /* Display label */
		NULL,                                     /* short-cut */
		N_("Delete all breakpoints"),             /* Tooltip */
		G_CALLBACK (on_clear_breakpoints1_activate)/* action callback */
	},
	{
		"ActionMenuGdbInformation",
		NULL,
		N_("_Info"),
		NULL,
		NULL,
		NULL
	},
	{
		"ActionGdbInfoTargetFiles",
		NULL,
		N_("Info _Target Files"),
		NULL,
		N_("Display information on the files the debugger is active with"),
		G_CALLBACK (on_info_targets_activate)
	},
	{
		"ActionGdbInfoProgram",
		NULL,
		N_("Info _Program"),
		NULL,
		N_("Display information on the execution status of the program"),
		G_CALLBACK (on_info_program_activate)
	},
	{
		"ActionGdbInfoKernelUserStruct",
		NULL,
		N_("Info _Kernel User Struct"),
		NULL,
		N_("Display the contents of kernel 'struct user' for current child"),
		G_CALLBACK (on_info_udot_activate)
	},
	{
		"ActionGdbInfoThreads",
		NULL,
		N_("Info _Threads"),
		NULL,
		N_("Display the IDs of currently known threads"),
		G_CALLBACK (on_info_threads_activate)
	},
	{
		"ActionGdbInfoGlobalVariables",
		NULL,
		N_("Info _Global variables"),
		NULL,
		N_("Display all global and static variables of the program"),
		G_CALLBACK (on_info_variables_activate)
	},
	{
		"ActionGdbInfoLocalVariables",
		NULL,
		N_("Info _Local variables"),
		NULL,
		N_("Display local variables of the current frame"),
		G_CALLBACK (on_info_locals_activate)
	},
	{
		"ActionGdbInfoCurrentFrame",
		NULL,
		N_("Info _Current Frame"),
		NULL,
		N_("Display information about the current frame of execution"),
		G_CALLBACK (on_info_frame_activate)
	},
	{
		"ActionGdbFunctionArgs",
		NULL,
		N_("Info Function _Arguments"),
		NULL,
		N_("Display function arguments of the current frame"),
		G_CALLBACK (on_info_args_activate)
	},
	{
		"ActionGdbInfoMemory",
		NULL,
		N_("Info _Memory"),
		NULL,
		N_("Display accessible memory"),
		G_CALLBACK (on_info_memory_activate)
	},
	{
		"ActionGdbRestartProgram",
		GTK_STOCK_REFRESH,
		N_("_Restart Program"),
		NULL,
		N_("Stop and restart the program"),
		G_CALLBACK (on_debugger_restart_prog_activate)
	},
	{
		"ActionGdbStopProgram",
		GTK_STOCK_STOP,
		N_("S_top Program"),
		NULL,
		N_("Stop the program being debugged"),
		G_CALLBACK (on_debugger_stop_prog_activate)
	},
	{
		"ActionGdbDetachProgram",
		NULL,
		N_("_Detach Debugger"),
		NULL,
		N_("Detach from an attached program"),
		G_CALLBACK (on_debugger_detach_activate)
	},
	{
		"ActionGdbPauseProgram",
		NULL,
		N_("Pa_use Program"),
		NULL,
		N_("Pauses the execution of the program"),
		G_CALLBACK (on_debugger_interrupt_activate)
	},
	{
		"ActionGdbSignalProgram",
		NULL,
		N_("Si_gnal to Process"),
		NULL,
		N_("Send a kernel signal to the process being debugged"),
		G_CALLBACK (on_debugger_signal_activate)
	},
	{
		"ActionGdbInspect",
		GTK_STOCK_DIALOG_INFO,
		N_("Ins_pect/Evaluate ..."),
		NULL,
		N_("Inspect or evaluate an expression or variable"),
		G_CALLBACK (on_debugger_inspect_activate)
	},
	{
		"ActionGdbAddWatch",
		NULL,
		N_("Add Expression in _Watch ..."),
		NULL,
		N_("Add expression or variable to the watch"),
		G_CALLBACK (on_debugger_add_watch_activate)
	},
	{
		"ActionGdbStop",
		GTK_STOCK_STOP,
		N_("St_op Debugger"),
		NULL,
		N_("Say goodbye to the debugger"),
		G_CALLBACK (on_debugger_stop_activate)
	}
};

static void
on_debug_buffer_flushed (IAnjutaMessageView *view, const gchar* line,
		AnjutaPlugin *plugin)
{
	g_return_if_fail (line != NULL);

	IAnjutaMessageViewType type = IANJUTA_MESSAGE_VIEW_TYPE_INFO;
	ianjuta_message_view_append (view, type, line, "", NULL);
}


static void
on_debug_mesg_clicked (IAnjutaMessageView* view, const gchar* line,
		AnjutaPlugin* plugin)
{
	/* TODO */
}

static void
value_added_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
							  const GValue *value, gpointer user_data)
{
	GdbPlugin *gdb_plugin;
	const gchar *root_uri;

	gdb_plugin = (GdbPlugin *) plugin;
	
	if (gdb_plugin->project_root_uri)
		g_free (gdb_plugin->project_root_uri);
	gdb_plugin->project_root_uri = NULL;
	
	root_uri = g_value_get_string (value);
	if (root_uri)
	{
		gdb_plugin->project_root_uri = g_strdup (root_uri);
	}
}

static void
value_removed_project_root_uri (AnjutaPlugin *plugin, const gchar *name,
								gpointer user_data)
{
	GdbPlugin *gdb_plugin;

	gdb_plugin = (GdbPlugin *) plugin;
	
	if (gdb_plugin->project_root_uri)
		g_free (gdb_plugin->project_root_uri);
	gdb_plugin->project_root_uri = NULL;
}

static void
value_added_current_editor (AnjutaPlugin *plugin, const char *name,
							const GValue *value, gpointer data)
{
	GObject *editor;
	GdbPlugin *gdb_plugin;
	
	editor = g_value_get_object (value);
	gdb_plugin = (GdbPlugin*)plugin;
	gdb_plugin->current_editor = editor;
}

static void
value_removed_current_editor (AnjutaPlugin *plugin,
							  const char *name, gpointer data)
{
	GdbPlugin *gdb_plugin = (GdbPlugin*)plugin;
	gdb_plugin->current_editor = NULL;
}

static gboolean
activate_plugin (AnjutaPlugin* plugin)
{
	AnjutaUI *ui;
	GObject *obj;
	IAnjutaMessageManager *message_manager;
	IAnjutaMessageView *message_view;
	GdbPlugin *gdb_plugin = (GdbPlugin *) plugin;

	DEBUG_PRINT ("GdbPlugin: Activating Gdb plugin ...");

	/* Query for object implementing IAnjutaMessageManager interface */
	obj = anjuta_shell_get_object (ANJUTA_PLUGIN (plugin)->shell,
			"IAnjutaMessageManager", NULL);
	message_manager = IANJUTA_MESSAGE_MANAGER (obj);

	/* TODO: error checking */
	message_view = ianjuta_message_manager_add_view (
			message_manager, MESSAGE_VIEW_TITLE, ICON_FILE, NULL);
	g_signal_connect (G_OBJECT (message_view), "buffer_flushed",
			G_CALLBACK (on_debug_buffer_flushed), plugin);
	g_signal_connect (G_OBJECT (message_view), "message_clicked",
			G_CALLBACK (on_debug_mesg_clicked), plugin);
	ianjuta_message_manager_set_current_view (message_manager, message_view, NULL);

	gdb_plugin->launcher = anjuta_launcher_new ();
	
	ui = anjuta_shell_get_ui (plugin->shell, NULL);
	
	/* Add action group */
	gdb_plugin->action_group = 
		anjuta_ui_add_action_group_entries (ui,
											"ActionGroupBuild",
											_("Build commands"),
											actions_gdb,
											sizeof(actions_gdb)/sizeof(GtkActionEntry),
											plugin);
	/* Add UI */
	gdb_plugin->merge_id = anjuta_ui_merge (ui, UI_FILE);
	
	/* Add watches */
	gdb_plugin->project_watch_id = 
		anjuta_plugin_add_watch (plugin, "project_root_uri",
								 value_added_project_root_uri,
								 value_removed_project_root_uri, NULL);
	gdb_plugin->editor_watch_id = 
		anjuta_plugin_add_watch (plugin, "document_manager_current_editor",
								 value_added_current_editor,
								 value_removed_current_editor, NULL);
	debugger_init (gdb_plugin);

	return TRUE;
}


static gboolean
deactivate_plugin (AnjutaPlugin* plugin)
{
	g_message ("GdbPlugin: Deactivating Gdb plugin ...");

	/* TODO: remove view */

	debugger_shutdown ();

	return TRUE;
}


static void
dispose (GObject* obj)
{
	GNOME_CALL_PARENT (G_OBJECT_CLASS, dispose, (obj));
}


static void
gdb_plugin_instance_init (GObject* obj)
{
	GdbPlugin *plugin = (GdbPlugin *) obj;
	plugin->uiid = 0;
	plugin->launcher = NULL;
	
	plugin->merge_id = 0;
	plugin->editor_watch_id = 0;
	plugin->project_watch_id = 0;
	
	plugin->current_editor = NULL;
	plugin->project_root_uri = NULL;
}


static void
gdb_plugin_class_init (GObjectClass* klass) 
{
	AnjutaPluginClass *plugin_class = ANJUTA_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	plugin_class->activate = activate_plugin;
	plugin_class->deactivate = deactivate_plugin;
	klass->dispose = dispose;
}


/* Implementation of IAnjutaDebugger interface */
static gboolean
idebugger_is_active (IAnjutaDebugger *plugin, GError **err)
{
	return debugger_is_active ();
}

static void
idebugger_start (IAnjutaDebugger *plugin, const gchar *prog, GError **err)
{
	debugger_start (prog);
}

static void
idebugger_load_executable (IAnjutaDebugger *plugin, const gchar *prog, GError **err)
{
	debugger_load_executable (prog);
}

static void
idebugger_load_core (IAnjutaDebugger *plugin, const gchar *core, GError **err)
{
	debugger_load_core (core);
}

static void
idebugger_run_continue (IAnjutaDebugger *plugin, GError **err)
{
	debugger_run ();
}

static void
idebugger_step_in (IAnjutaDebugger *plugin, GError **err)
{
	debugger_step_in ();
}

static void
idebugger_step_over (IAnjutaDebugger *plugin, GError **err)
{
	debugger_step_over ();
}

static void
idebugger_run_to_cursor (IAnjutaDebugger *plugin, GError **err)
{
	guint line;
	gchar *uri;
	GdbPlugin *gdb_plugin;
	GnomeVFSURI *vfs_uri;
	
	gdb_plugin = (GdbPlugin*)G_OBJECT (plugin);
	
	if (debugger_is_active() == FALSE) return;
	if (debugger_is_ready() == FALSE) return;

	g_return_if_fail (IANJUTA_IS_EDITOR (gdb_plugin->current_editor));
	uri = ianjuta_file_get_uri (IANJUTA_FILE (gdb_plugin->current_editor), NULL);
	if (!uri)
		return;
	vfs_uri = gnome_vfs_uri_new (uri);
	
	g_return_if_fail (vfs_uri != NULL);
	
	if (gnome_vfs_uri_is_local (vfs_uri))
	{
		const gchar *filename;
		gchar *buff;
		
		filename = gnome_vfs_uri_get_path (vfs_uri);
		line = ianjuta_editor_get_lineno (IANJUTA_EDITOR (gdb_plugin->current_editor),
										  NULL);
	
		buff = g_strdup_printf ("%s:%d", filename, line);
		debugger_run_to_location (buff);
		g_free (buff);
	}
	gnome_vfs_uri_unref (vfs_uri);
	g_free (uri);
}

static void
idebugger_step_out (IAnjutaDebugger *plugin, GError **err)
{
	debugger_step_out ();
}

static void
idebugger_toggle_breakpoint (IAnjutaDebugger *plugin, GError **err)
{
	debugger_toggle_breakpoint (0); // 0 means current line
}

static void
idebugger_toggle_breakpoint1 (IAnjutaDebugger *plugin, gint line,
							  GError **err)
{
	debugger_toggle_breakpoint (line);
}

static void
idebugger_iface_init (IAnjutaDebuggerIface *iface)
{
	iface->is_active = idebugger_is_active;
	iface->start = idebugger_start;
	iface->load_executable = idebugger_load_executable;
	iface->load_core = idebugger_load_core;
	iface->run_continue = idebugger_run_continue;
	iface->step_in = idebugger_step_in;
	iface->step_over = idebugger_step_over;
	iface->step_out = idebugger_step_out;
	iface->run_to_cursor = idebugger_run_to_cursor;
	iface->toggle_breakpoint = idebugger_toggle_breakpoint;
	iface->toggle_breakpoint1 = idebugger_toggle_breakpoint1;
}

ANJUTA_PLUGIN_BEGIN (GdbPlugin, gdb_plugin);
ANJUTA_PLUGIN_ADD_INTERFACE(idebugger, IANJUTA_TYPE_DEBUGGER);
ANJUTA_PLUGIN_END;

ANJUTA_SIMPLE_PLUGIN (GdbPlugin, gdb_plugin);
