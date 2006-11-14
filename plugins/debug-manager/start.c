/*
    start.c
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

/*
 * Handle start of debugger. It can be done in three ways:
 *  - Load a target of the current project_root_uri
 *  - Load a executable file
 *  - Attach to an already running program
 *---------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "start.h"

#include <libanjuta/resources.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <gnome.h>

#include <glade/glade-xml.h>

#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

/* Types
 *---------------------------------------------------------------------------*/

typedef struct _AttachProcess AttachProcess;
typedef struct _LoadFileCallBack LoadFileCallBack;
	
enum
{
	CLEAR_INITIAL,
	CLEAR_UPDATE,
	CLEAR_REVIEW,
	CLEAR_FINAL
};
	
struct _AttachProcess
{
    GtkWidget*  dialog;
    GtkWidget*  treeview;
	gint        pid;

	gboolean    hide_paths;
	gboolean    hide_params;
	gboolean    process_tree;

	gchar*      ps_output;
	GSList*     iter_stack;
	gint        iter_stack_level;
	gint        num_spaces_to_skip;
	gint        num_spaces_per_level;
};

enum {
	PID_COLUMN,
	USER_COLUMN,
	START_COLUMN,
	COMMAND_COLUMN,
	COLUMNS_NB
};

static char *column_names[COLUMNS_NB] = {
	N_("Pid"), N_("User"), N_("Time"), N_("Command")
};

struct _LoadFileCallBack
{
	AnjutaPlugin *plugin;
	IAnjutaDebugger *debugger;
};

struct _DmaStart
{
	AnjutaPlugin *plugin;

	IAnjutaDebugger *debugger;

	gchar* target_uri;
	gchar* program_args;
	gboolean run_in_terminal;
};

/* Widgets found in glade file
 *---------------------------------------------------------------------------*/

#define PARAMETER_DIALOG "parameter_dialog"
#define TERMINAL_CHECK_BUTTON "parameter_run_in_term_check"
#define PARAMETER_COMBO "parameter_combo"
#define TARGET_COMBO "target_combo"
#define TARGET_SELECT_SIGNAL "on_select_target_clicked"

#define ANJUTA_RESPONSE_SELECT_TARGET 0

static void attach_process_clear (AttachProcess * ap, gint ClearRequest);

/* Helper functions
 *---------------------------------------------------------------------------*/

/* This functions get all directories of the current project containing
 * static or shared library. Perhaps a more reliable way to find these
 * directories is to really get the directories of all source files */

static GList*
get_source_directories (AnjutaPlugin *plugin)
{
	gchar *cwd;
	GList *node, *search_dirs = NULL;
	GList *slibs_dirs = NULL;
	GList *libs_dirs = NULL;
	GValue value = {0,};
	
	return NULL;
	cwd = g_get_current_dir();
	search_dirs = g_list_prepend (search_dirs, g_strconcat ("file://", cwd, NULL));
	g_free (cwd);

	/* Check if a project is already open */
	anjuta_shell_get_value (plugin->shell, "project_root_uri", &value, NULL);
	
	/* Set source file search directories */
	if (g_value_get_string (&value) != NULL)
	{
		IAnjutaProjectManager *pm;
		pm = anjuta_shell_get_interface (plugin->shell,
										 IAnjutaProjectManager, NULL);
		if (pm)
		{
			slibs_dirs =
				ianjuta_project_manager_get_targets (pm,
					IANJUTA_PROJECT_MANAGER_TARGET_SHAREDLIB,
												  NULL);
			libs_dirs =
				ianjuta_project_manager_get_targets (pm,
					IANJUTA_PROJECT_MANAGER_TARGET_STATICLIB,
												  NULL);
		}
	}
	slibs_dirs = g_list_reverse (slibs_dirs);
	libs_dirs = g_list_reverse (libs_dirs);
	
	node = slibs_dirs;
	while (node)
	{
		gchar *dir_uri;
		dir_uri = g_path_get_dirname (node->data);
		search_dirs = g_list_prepend (search_dirs, dir_uri);
		node = g_list_next (node);
	}
	
	node = libs_dirs;
	while (node)
	{
		gchar *dir_uri;
		dir_uri = g_path_get_dirname (node->data);
		search_dirs = g_list_prepend (search_dirs, dir_uri);
		node = g_list_next (node);
	}
	
	g_list_foreach (slibs_dirs, (GFunc)g_free, NULL);
	g_list_free (slibs_dirs);
	g_list_foreach (libs_dirs, (GFunc)g_free, NULL);
	g_list_free (libs_dirs);
	
	return g_list_reverse (search_dirs);
}

static void
free_source_directories (GList *dirs)
{
	g_list_foreach (dirs, (GFunc)g_free, NULL);
	g_list_free (dirs);
}

/* Callback for saving session
 *---------------------------------------------------------------------------*/

static void
on_session_save (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, DmaStart *this)
{
	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	if (this->program_args)
	{
		gchar *arg;
		anjuta_session_set_string (session, "Execution", "Program arguments", this->program_args);
		arg = anjuta_session_get_string (session, "Execution", "Program arguments");
	}
	if (this->target_uri)
	{
		gchar *uri;
		anjuta_session_set_string (session, "Execution", "Program uri", this->target_uri);
		uri = anjuta_session_get_string (session, "Execution", "Program uri");
	}
	anjuta_session_set_int (session, "Execution", "Run in terminal", this->run_in_terminal + 1);
}

static void on_session_load (AnjutaShell *shell, AnjutaSessionPhase phase, AnjutaSession *session, DmaStart *this)
{
	gchar *program_args;
	gchar *target_uri;
    gint run_in_terminal;

	if (phase != ANJUTA_SESSION_PHASE_NORMAL)
		return;

	program_args = anjuta_session_get_string (session, "Execution", "Program arguments");
	if (this->program_args)
	{
		g_free (this->program_args);
		this->program_args = NULL;
	}
    if (program_args)
    {
		this->program_args = program_args;
    }

	target_uri = anjuta_session_get_string (session, "Execution", "Program uri");
	if (this->target_uri)
	{
		g_free (this->target_uri);
		this->target_uri = NULL;
	}
    if (target_uri)
    {
		this->target_uri = target_uri;
    }
	
	/* The flag is store as 1 == FALSE, 2 == TRUE */
    run_in_terminal = anjuta_session_get_int (session, "Execution", "Run in terminal");
	run_in_terminal--;

    if (run_in_terminal >= 0)
		this->run_in_terminal = run_in_terminal;
}

/* Attach to process private functions
 *---------------------------------------------------------------------------*/

static AttachProcess *
attach_process_new ()
{
	AttachProcess *ap;
	ap = g_new0 (AttachProcess, 1);
	attach_process_clear (ap, CLEAR_INITIAL);
	return ap;
}

static void
attach_process_clear (AttachProcess * ap, gint ClearRequest)
{
	GtkTreeModel *model;

	// latest ps output
	switch (ClearRequest)
	{
	case CLEAR_UPDATE:
	case CLEAR_FINAL:
		if (ap->ps_output)
		{
			g_free (ap->ps_output);
		}
	case CLEAR_INITIAL:
		ap->ps_output = NULL;
	}

	// helper variables
	switch (ClearRequest)
	{
	case CLEAR_INITIAL:
	case CLEAR_UPDATE:
	case CLEAR_REVIEW:
		ap->pid = -1L;
		ap->iter_stack = NULL;
		ap->iter_stack_level = -1;
		ap->num_spaces_to_skip = -1;
	}

	// tree model
	switch (ClearRequest)
	{
	case CLEAR_UPDATE:
	case CLEAR_REVIEW:
	case CLEAR_FINAL:
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (ap->treeview));
		gtk_tree_store_clear (GTK_TREE_STORE (model));
	}

	// dialog widget
	if (ClearRequest == CLEAR_FINAL)
	{
		gtk_widget_destroy (ap->dialog);
		ap->dialog = NULL;
	}
}

static inline gchar *
skip_spaces (gchar *pos)
{
	while (*pos == ' ')
		pos++;
	return pos;
}

static inline gchar *
skip_token (gchar *pos)
{
	while (*pos != ' ')
		pos++;
	*pos++ = '\0';
	return pos;
}

static gchar *
skip_token_and_spaces (gchar *pos)
{
	pos = skip_token (pos);
	return skip_spaces (pos);
}

static GtkTreeIter *
iter_stack_push_new (AttachProcess *ap, GtkTreeStore *store)
{
	GtkTreeIter *new_iter, *top_iter;
	new_iter = g_new (GtkTreeIter, 1);
	top_iter = (GtkTreeIter *) (g_slist_nth_data (ap->iter_stack, 0));
	ap->iter_stack =
			g_slist_prepend (ap->iter_stack, (gpointer) (new_iter));
	gtk_tree_store_append (store, new_iter, top_iter);
	ap->iter_stack_level++;
	return new_iter;
}

static gboolean
iter_stack_pop (AttachProcess *ap)
{
	if (ap->iter_stack_level < 0)
		return FALSE;

	GtkTreeIter *iter =
			(GtkTreeIter *) (g_slist_nth_data (ap->iter_stack, 0));
	ap->iter_stack =
			g_slist_delete_link (ap->iter_stack, ap->iter_stack);
	g_free (iter);
	ap->iter_stack_level--;
	return TRUE;
}

static void
iter_stack_clear (AttachProcess *ap)
{
	while (iter_stack_pop (ap));
}

static gchar *
calc_depth_and_get_iter (AttachProcess *ap, GtkTreeStore *store,
			GtkTreeIter **iter, gchar *pos)
{
	gchar *orig_pos;
	guint num_spaces, depth, i;

	// count spaces
	orig_pos = pos;
	while (*pos == ' ')
		pos++;
	num_spaces = pos - orig_pos;

	if (ap->process_tree)
	{
		if (ap->num_spaces_to_skip < 0)
		{
			// first process to be inserted
			ap->num_spaces_to_skip = num_spaces;
			ap->num_spaces_per_level = -1;
			*iter = iter_stack_push_new (ap, store);
		}
		else
		{
			if (ap->num_spaces_per_level < 0)
			{
				if (num_spaces == ap->num_spaces_to_skip)
				{
					// num_spaces_per_level still unknown
					iter_stack_pop (ap);
					*iter = iter_stack_push_new (ap, store);
				}
				else
				{
					// first time at level 1
					ap->num_spaces_per_level = 
							num_spaces - ap->num_spaces_to_skip;
					*iter = iter_stack_push_new (ap, store);
				}
			}
			else
			{
				depth = (num_spaces - ap->num_spaces_to_skip) /
						ap->num_spaces_per_level;
				if (depth == ap->iter_stack_level)
				{
					// level not changed
					iter_stack_pop (ap);
					*iter = iter_stack_push_new (ap, store);
				}
				else
					if (depth == ap->iter_stack_level + 1)
						*iter = iter_stack_push_new (ap, store);
					else
						if (depth < ap->iter_stack_level)
						{
							// jump some levels backward
							depth = ap->iter_stack_level - depth;
							for (i = 0; i <= depth; i++)
								iter_stack_pop (ap);
							*iter = iter_stack_push_new (ap, store);
						}
						else
						{
							// should never get here
							g_warning("Unknown error");
							iter_stack_pop (ap);
							*iter = iter_stack_push_new (ap, store);
						}
			}
		}
	}
	else
	{
		iter_stack_pop (ap);
		*iter = iter_stack_push_new (ap, store);
	}

	return pos;
}

static gchar *
skip_path (gchar *pos)
{
	/* can't use g_path_get_basename() - wouldn't work for a processes
	started with parameters containing '/' */
	gchar c, *final_pos = pos;

	if (*pos == G_DIR_SEPARATOR)
		do
		{
			c = *pos;
			if (c == G_DIR_SEPARATOR)
			{
				final_pos = ++pos;
				continue;
			}
			else
				if (c == ' ' || c == '\0')
					break;
				else
					++pos;
		}
		while (1);

	return final_pos;
}

static inline void
remove_params (gchar *pos)
{
	do
	{
		if (*(++pos) == ' ')
			*pos = '\0';
	}
	while (*pos);
}

static void
attach_process_add_line (AttachProcess *ap, GtkTreeStore *store, gchar *line)
{
	gchar *pid, *user, *start, *command, *tmp;
	// guint i, level;
	GtkTreeIter *iter;

	pid = skip_spaces (line); // skip leading spaces
	user = skip_token_and_spaces (pid); // skip PID
	start = skip_token_and_spaces (user); // skip USER
	tmp = skip_token (start); // skip START (do not skip spaces)

	command = calc_depth_and_get_iter (ap, store, &iter, tmp);

	if (ap->hide_paths)
	{
		command = skip_path (command);
	}

	if (ap->hide_params)
	{
		remove_params(command);
	}

	gtk_tree_store_set (store, iter,
						PID_COLUMN, pid,
						USER_COLUMN, user,
						START_COLUMN, start,
						COMMAND_COLUMN, command,
						-1);
}

static void
attach_process_review (AttachProcess *ap)
{
	gchar *ps_output, *begin, *end;
	guint line_num = 0;
	GtkTreeStore *store;

	g_return_if_fail (ap);
	g_return_if_fail (ap->ps_output);
	store = GTK_TREE_STORE (gtk_tree_view_get_model 
							(GTK_TREE_VIEW (ap->treeview)));
	g_return_if_fail (store);

	ps_output = g_strdup (ap->ps_output);
	end = ps_output;
	while (*end)
	{
		begin = end;
		while (*end && *end != '\n') end++;
		if (++line_num > 2) // skip description line & process 'init'
		{
			*end = '\0';
			attach_process_add_line (ap, store, begin);
		}
		end++;
	}
	g_free (ps_output);

	iter_stack_clear (ap);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (ap->treeview));
}

static void
attach_process_update (AttachProcess * ap)
{
	gchar *tmp, *tmp1, *cmd;
	gint ch_pid;
	gchar *shell;
	GtkTreeStore *store;
	gboolean result;

	g_return_if_fail (ap);
	store = GTK_TREE_STORE (gtk_tree_view_get_model
							(GTK_TREE_VIEW (ap->treeview)));
	g_return_if_fail (store);
	
	if (anjuta_util_prog_is_installed ("ps", TRUE) == FALSE)
		return;

	tmp = anjuta_util_get_a_tmp_file ();
	cmd = g_strconcat ("ps axw -H -o pid,user,start_time,args > ", tmp, NULL);
	shell = gnome_util_user_shell ();
	ch_pid = fork ();
	if (ch_pid == 0)
	{
		execlp (shell, shell, "-c", cmd, NULL);
	}
	if (ch_pid < 0)
	{
		anjuta_util_dialog_error_system (NULL, errno,
										 _("Unable to execute: %s."), cmd);
		g_free (tmp);
		g_free (cmd);
		return;
	}
	waitpid (ch_pid, NULL, 0);
	g_free (cmd);

	result = g_file_get_contents (tmp, &tmp1, NULL, NULL);
	remove (tmp);
	g_free (tmp);
	if (! result)
	{
		anjuta_util_dialog_error_system (NULL, errno,
										 _("Unable to open the file: %s\n"),
										 tmp);
		return;
	}

	attach_process_clear (ap, CLEAR_UPDATE);
	ap->ps_output = anjuta_util_convert_to_utf8 (tmp1);
	g_free (tmp1);
	if (ap->ps_output)
	{
		attach_process_review (ap);
	}
}

static void
on_selection_changed (GtkTreeSelection *selection, AttachProcess *ap)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	g_return_if_fail (ap);
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar* text;
		gtk_tree_model_get (model, &iter, PID_COLUMN, &text, -1);
		ap->pid = atoi (text);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (ap->dialog), 
										   GTK_RESPONSE_OK, TRUE);
	}
	else
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (ap->dialog), 
										   GTK_RESPONSE_OK, FALSE);
		ap->pid = -1L;
	}
}

static gboolean
on_delete_event (GtkWidget *dialog, GdkEvent *event, AttachProcess *ap)
{
	g_return_val_if_fail (ap, FALSE);
	attach_process_clear (ap, CLEAR_FINAL);
	return FALSE;
}

static gint
sort_pid (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
		gpointer user_data)
{
	gchar *nptr;
	gint pid1, pid2;

	gtk_tree_model_get (model, a, PID_COLUMN, &nptr, -1);
	pid1 = atoi (nptr);

	gtk_tree_model_get (model, b, PID_COLUMN, &nptr, -1);
	pid2 = atoi (nptr);

	return pid1 - pid2;
}

static void
on_toggle_hide_paths (GtkToggleButton *togglebutton, AttachProcess * ap)
{
	ap->hide_paths = gtk_toggle_button_get_active (togglebutton);
	attach_process_clear (ap, CLEAR_REVIEW);
	attach_process_review (ap);
}

static void
on_toggle_hide_params (GtkToggleButton *togglebutton, AttachProcess * ap)
{
	ap->hide_params = gtk_toggle_button_get_active (togglebutton);
	attach_process_clear (ap, CLEAR_REVIEW);
	attach_process_review (ap);
}

static void
on_toggle_process_tree (GtkToggleButton *togglebutton, AttachProcess * ap)
{
	ap->process_tree = gtk_toggle_button_get_active (togglebutton);
	attach_process_clear (ap, CLEAR_REVIEW);
	attach_process_review (ap);
}

static pid_t
attach_process_show (AttachProcess * ap, GtkWindow *parent)
{
	GladeXML *gxml;
	GtkTreeView *view;
	GtkTreeStore *store;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkCheckButton *checkb_hide_paths;
	GtkCheckButton *checkb_hide_params;
	GtkCheckButton *checkb_process_tree;
	gint i, res;
	pid_t selected_pid = -1;
	
	g_return_val_if_fail (ap != NULL, -1);

	if (!ap->dialog)
	{
		gxml = glade_xml_new (GLADE_FILE, "attach_process_dialog", NULL);
		ap->dialog = glade_xml_get_widget (gxml, "attach_process_dialog");
		ap->treeview = glade_xml_get_widget (gxml, "attach_process_tv");
		checkb_hide_paths = GTK_CHECK_BUTTON (
								glade_xml_get_widget (gxml, "checkb_hide_paths"));
		checkb_hide_params = GTK_CHECK_BUTTON (
								glade_xml_get_widget (gxml, "checkb_hide_params"));
		checkb_process_tree = GTK_CHECK_BUTTON (
								glade_xml_get_widget (gxml, "checkb_process_tree"));
		g_object_unref (gxml);
	
		view = GTK_TREE_VIEW (ap->treeview);
		store = gtk_tree_store_new (COLUMNS_NB,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING);
		gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
						 GTK_SELECTION_SINGLE);
		g_object_unref (G_OBJECT (store));
	
		renderer = gtk_cell_renderer_text_new ();
	
		for (i = PID_COLUMN; i < COLUMNS_NB; i++) {
			GtkTreeViewColumn *column;
	
			column = gtk_tree_view_column_new_with_attributes (column_names[i],
														renderer, "text", i, NULL);
			gtk_tree_view_column_set_sort_column_id(column, i);
			gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
			gtk_tree_view_append_column (view, column);
			if (i == COMMAND_COLUMN)
				gtk_tree_view_set_expander_column(view, column);
		}
		gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store), PID_COLUMN,
						sort_pid, NULL, NULL);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
						START_COLUMN, GTK_SORT_DESCENDING);
	
		ap->hide_paths = gtk_toggle_button_get_active (
							GTK_TOGGLE_BUTTON (checkb_hide_paths));
		ap->hide_params = gtk_toggle_button_get_active (
							GTK_TOGGLE_BUTTON (checkb_hide_params));
		ap->process_tree = gtk_toggle_button_get_active (
							GTK_TOGGLE_BUTTON (checkb_process_tree));
	
		attach_process_update (ap);
	
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ap->treeview));
		g_signal_connect (G_OBJECT (selection), "changed",
						  G_CALLBACK (on_selection_changed), ap);
		g_signal_connect (G_OBJECT (ap->dialog), "delete_event",
						  G_CALLBACK (on_delete_event), ap);
		g_signal_connect (GTK_OBJECT (checkb_hide_paths), "toggled",
							G_CALLBACK (on_toggle_hide_paths), ap);
		g_signal_connect (GTK_OBJECT (checkb_hide_params), "toggled",
							G_CALLBACK (on_toggle_hide_params), ap);
		g_signal_connect (GTK_OBJECT (checkb_process_tree), "toggled",
							G_CALLBACK (on_toggle_process_tree), ap);
	}
	
	gtk_window_set_transient_for (GTK_WINDOW (ap->dialog),
								  GTK_WINDOW (parent));
	/* gtk_widget_show (ap->dialog); */
	res = gtk_dialog_run (GTK_DIALOG (ap->dialog));
	while (res == GTK_RESPONSE_APPLY)
	{
		attach_process_update (ap);
		res = gtk_dialog_run (GTK_DIALOG (ap->dialog));
	}
	if (res == GTK_RESPONSE_OK)
	{
		selected_pid = ap->pid;
	}
	attach_process_clear (ap, CLEAR_FINAL);
	return selected_pid;
}

static void
attach_process_destroy (AttachProcess * ap)
{
	g_return_if_fail (ap);
	g_free (ap);
}

/* Load file private functions
 *---------------------------------------------------------------------------*/

static void
on_select_target (GtkButton *button, gpointer user_data)
{
	gtk_dialog_response (GTK_DIALOG (user_data), ANJUTA_RESPONSE_SELECT_TARGET);
}

static void
dma_start_load_uri (DmaStart *this)
{
	GList *search_dirs;
	GnomeVFSURI *vfs_uri;
	gchar *mime_type;
	const gchar *filename;
	// GList *node;

	if ((this->target_uri != NULL) && (*(this->target_uri) != '\0'))
	{
		vfs_uri = gnome_vfs_uri_new (this->target_uri);
		
		g_return_if_fail (vfs_uri != NULL);
		
		if (!gnome_vfs_uri_is_local (vfs_uri)) return;
			
		search_dirs = get_source_directories (this->plugin);
		
		mime_type = gnome_vfs_get_mime_type (this->target_uri);
	        filename = gnome_vfs_uri_get_path (vfs_uri);

		ianjuta_debugger_interrupt (this->debugger, NULL);
		ianjuta_debugger_quit (this->debugger, NULL);
		ianjuta_debugger_load (this->debugger, filename, mime_type, search_dirs, NULL);
		
		g_free (mime_type);
		gnome_vfs_uri_unref (vfs_uri);
		free_source_directories (search_dirs);

	}
}

/* Public functions
 *---------------------------------------------------------------------------*/

void
dma_attach_to_process (DmaStart* this)
{
	pid_t selected_pid;
	GtkWindow *parent;
	AttachProcess *attach_process = NULL;
	
	parent = GTK_WINDOW (ANJUTA_PLUGIN (this->plugin)->shell);
	attach_process = attach_process_new();
	
	selected_pid = attach_process_show (attach_process, parent);
	if (selected_pid > 0)
	{
		long lpid = selected_pid;
		GList *search_dirs;
		
		search_dirs = get_source_directories (this->plugin);
		ianjuta_debugger_interrupt (this->debugger, NULL);
		ianjuta_debugger_quit (this->debugger, NULL);
		ianjuta_debugger_attach (this->debugger, lpid, search_dirs, NULL);
		free_source_directories (search_dirs);
	}
	attach_process_destroy(attach_process);
}

void
dma_run_target (DmaStart *this)
{
	if (this->target_uri == NULL)
	{
		if (dma_set_parameters (this) == FALSE) return;
	}

	dma_start_load_uri (this);
	ianjuta_debugger_start (this->debugger, this->program_args == NULL ? "" : this->program_args, this->run_in_terminal, NULL);
	
	return;
}

gboolean
dma_set_parameters (DmaStart *this)
{
	GladeXML *gxml;
	GtkWidget *dlg;
	GtkWindow *parent;
	GtkToggleButton *term;
	GtkComboBox *params;
	GtkComboBox *target;
	gint response;
	GtkTreeModel* model;
	GtkTreeIter iter;
	GValue value = {0,};
	const gchar *project_root_uri;

	
	
	parent = GTK_WINDOW (this->plugin->shell);
	gxml = glade_xml_new (GLADE_FILE, PARAMETER_DIALOG, NULL);
	if (gxml == NULL)
	{
		anjuta_util_dialog_error(parent, _("Missing file %s"), GLADE_FILE);
		return FALSE;
	}
		
	dlg = glade_xml_get_widget (gxml, PARAMETER_DIALOG);
	term = GTK_TOGGLE_BUTTON (glade_xml_get_widget (gxml, TERMINAL_CHECK_BUTTON));
	params = GTK_COMBO_BOX (glade_xml_get_widget (gxml, PARAMETER_COMBO));
	target = GTK_COMBO_BOX (glade_xml_get_widget (gxml, TARGET_COMBO));
	glade_xml_signal_connect_data (gxml, TARGET_SELECT_SIGNAL, GTK_SIGNAL_FUNC (on_select_target), dlg);
	g_object_unref (gxml);

	/* Fill parameter combo box */
	model = GTK_TREE_MODEL (gtk_list_store_new (1, GTK_TYPE_STRING));
	gtk_combo_box_set_model (params, model);
	gtk_combo_box_entry_set_text_column( GTK_COMBO_BOX_ENTRY(params), 0);
	if (this->program_args != NULL)
	{
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, this->program_args, -1);
		gtk_entry_set_text (GTK_ENTRY (GTK_BIN (params)->child), this->program_args);
	}
	g_object_unref (model);

	/* Fill target combo box */
	model = GTK_TREE_MODEL (gtk_list_store_new (1, GTK_TYPE_STRING));
	gtk_combo_box_set_model (target, model);
	gtk_combo_box_entry_set_text_column( GTK_COMBO_BOX_ENTRY(target), 0);
	if (this->target_uri != NULL)
	{
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, this->target_uri, -1);
		gtk_entry_set_text (GTK_ENTRY (GTK_BIN (target)->child), this->target_uri);
	}

    anjuta_shell_get_value (this->plugin->shell, "project_root_uri", &value, NULL);
    project_root_uri = g_value_get_string (&value);
	if (project_root_uri != NULL)
	{
		/* One project loaded, get all executable target */
		IAnjutaProjectManager *pm;
		GList *exec_targets = NULL;
			
		pm = anjuta_shell_get_interface (this->plugin->shell,
										 IAnjutaProjectManager, NULL);
		if (pm != NULL)
		{
			exec_targets = ianjuta_project_manager_get_targets (pm,
							 IANJUTA_PROJECT_MANAGER_TARGET_EXECUTABLE,
							 NULL);
		}
		if (exec_targets != NULL)
		{
			GList *node;

			for (node = exec_targets; node; node = g_list_next (node))
			{
				gtk_list_store_append (GTK_LIST_STORE (model), &iter);
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, node->data, -1);
				g_free (node->data);
				node = g_list_next (node);
			}
			g_list_free (exec_targets);
		}
	}
	
	g_object_unref (model);

	/* Set terminal option */	
	if (this->run_in_terminal) gtk_toggle_button_set_active (term, TRUE);
	
	gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);
			
	/* Run dialog */
	for (;;)
	{
		response = gtk_dialog_run (GTK_DIALOG (dlg));
		switch (response)
		{
		case GTK_RESPONSE_OK:
		case GTK_RESPONSE_APPLY:
		{
			const gchar *arg;
			const gchar *uri;
			
			if (this->program_args != NULL) g_free (this->program_args);
			arg = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (params)->child));
			this->program_args = g_strdup (arg);
			
			uri = gtk_entry_get_text (GTK_ENTRY (GTK_BIN (target)->child));
			if (this->target_uri != NULL)
			{
				if (strcmp(uri, this->target_uri) != 0)
				{
					g_free (this->target_uri);
					this->target_uri = g_strdup (uri);
				}
			}
			else
			{
				this->target_uri = g_strdup (uri);
			}
			
			this->run_in_terminal = gtk_toggle_button_get_active (term);
			break;
		}
		case ANJUTA_RESPONSE_SELECT_TARGET:
		{
			GtkWidget *sel_dlg = gtk_file_chooser_dialog_new (
				_("Load Target to debug"), GTK_WINDOW (dlg),
				 GTK_FILE_CHOOSER_ACTION_OPEN,
				 GTK_STOCK_OPEN, GTK_RESPONSE_OK,
				 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
			gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(sel_dlg), FALSE);
			gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (sel_dlg), TRUE);

			GtkFileFilter *filter = gtk_file_filter_new ();
			gtk_file_filter_set_name (filter, _("All files"));
			gtk_file_filter_add_pattern (filter, "*");
			gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (sel_dlg), filter);

			//gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);
			
			response = gtk_dialog_run (GTK_DIALOG (sel_dlg));

			if (response == GTK_RESPONSE_OK)
			{
				gchar* uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (sel_dlg));
				
				if (this->target_uri != NULL)
				{
					if (strcmp(uri, this->target_uri) != 0)
					{
						g_free (this->target_uri);
						this->target_uri = uri;
					}
					else
					{
						g_free (uri);
					}
				}
				else
				{
					this->target_uri = uri;
				}
					
				gtk_entry_set_text (GTK_ENTRY (GTK_BIN (target)->child), this->target_uri);

			}
			gtk_widget_destroy (GTK_WIDGET (sel_dlg));

			continue;
		}
		default:
			break;
		}
		break;
	}
	gtk_widget_destroy (dlg);
	
	if (response == GTK_RESPONSE_APPLY)
	{
		return TRUE;
	}
	else if (response == GTK_RESPONSE_OK)
	{
		dma_start_load_uri (this);
		ianjuta_debugger_start (this->debugger, this->program_args == NULL ? "" : this->program_args, this->run_in_terminal, NULL);
	}
	
	return FALSE;
}

/* Constructor & Destructor
 *---------------------------------------------------------------------------*/

DmaStart *
dma_start_new (AnjutaPlugin *plugin, IAnjutaDebugger *debugger)
{
	DmaStart *this;
	
	this = g_new0 (DmaStart, 1);

	this->plugin = plugin;
	this->debugger = debugger;
	
	g_signal_connect (plugin->shell, "save-session",
					  G_CALLBACK (on_session_save), this);
    g_signal_connect (plugin->shell, "load-session",
					  G_CALLBACK (on_session_load), this);
	
	return this;
}

void
dma_start_free (DmaStart *this)
{
	g_signal_handlers_disconnect_by_func (this->plugin->shell, G_CALLBACK (on_session_save), this);
    g_signal_handlers_disconnect_by_func (this->plugin->shell, G_CALLBACK (on_session_load), this);
	if (this->program_args != NULL) g_free (this->program_args);
	if (this->target_uri != NULL) g_free (this->target_uri);
	g_free (this);
}

