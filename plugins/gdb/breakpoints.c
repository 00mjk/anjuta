/*
    breakpoints.c
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <gnome.h>

#include <libanjuta/resources.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-markable.h>

/* TODO #include "anjuta.h" */
#include "breakpoints.h"
/* TODO #include "controls.h" */
#include "utilities.h"
#include "debugger.h"
#include "plugin.h"

#define	BKPT_FIELDS	(13)

typedef enum _BreakpointType BreakpointType;
typedef struct _BreakpointItem BreakpointItem;
typedef struct _Properties Properties;

enum _BreakpointType
{
	breakpoint,
	watchpoint,
	catchpoint
};

struct _BreakpointItem
{
	BreakpointsDBase *bd;
	gboolean is_editing;
	gint id;
	gchar *disp;
	gboolean enable;
	gulong addr;
	gint pass;
	gchar *condition;
	gchar *file;
	guint line;
	gint handle;
	gboolean handle_invalid;
	gchar *function;
	gchar *info;
	time_t time;
};

struct _BreakpointsDBasePriv
{
	AnjutaPlugin *plugin;

	GladeXML *gxml;
	gchar *cond_history, *loc_history, *pass_history;
	
	gboolean is_showing;
	gint win_pos_x, win_pos_y, win_width, win_height;

	/* Widgets */	
	GtkWidget *window;
	GtkWidget *treeview;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *jumpto_button;
	GtkWidget *properties_button;
	GtkWidget *removeall_button;
	GtkWidget *enableall_button;
	GtkWidget *disableall_button;
};

struct _Properties
{
	BreakpointsDBase *bd;
	GladeXML *gxml;
	BreakpointItem *bid;
	GtkWidget *dialog;
};

enum {
	ENABLED_COLUMN,
	FILENAME_COLUMN,
	LINENO_COLUMN,
	FUNCTION_COLUMN,
	PASS_COLUMN,
	CONDITION_COLUMN,
	DATA_COLUMN,
	COLUMNS_NB
};

static char *column_names[COLUMNS_NB] = {
	N_("Enabled"), N_("File"), N_("Line"),
	N_("Function"), N_("Pass"), N_("Condition")
};


void breakpoints_info (GList * outputs, gpointer data );
void enable_all_breakpoints (BreakpointsDBase* bd);
void disable_all_breakpoints (BreakpointsDBase* bd);
void delete_breakpoint (gint id, BreakpointsDBase* bd, gboolean end);
void property_add_item (GList * outputs, gpointer data );
void property_destroy_breakpoint (BreakpointsDBase* bd, GladeXML *gxml,
								  BreakpointItem *bid, GtkWidget *dialog);
void enable_breakpoint (gint id, BreakpointsDBase* bd);
void disable_breakpoint (gint id, BreakpointsDBase* bd);


static gboolean bk_item_add (BreakpointsDBase *bd, GladeXML *gxml,
							 BreakpointItem *bid);

/* TODO
static void breakpoint_item_save (BreakpointItem *bi, ProjectDBase *pdb,
								  const gint nBreak );
*/
static void breakpoints_dbase_delete_all_breakpoints (BreakpointsDBase *bd);
static void breakpoints_dbase_delete_all_markers (BreakpointsDBase *bd,
		IAnjutaMarkableMarker marker);
static gboolean breakpoint_item_load (BreakpointItem *bi, gchar *szStr );
static void on_treeview_enabled_toggled (GtkCellRendererToggle *cell,
										 gchar *path_str, gpointer data);
static void breakpoints_dbase_add_brkpnt (BreakpointsDBase * bd,
										  gchar * brkpnt);
static gboolean breakpoints_dbase_set_from_item (BreakpointsDBase *bd,
												 BreakpointItem *bi,
												 gboolean add_to_treeview);


static IAnjutaDocumentManager *
get_document_manager (AnjutaPlugin *plugin)
{
	GObject *obj = anjuta_shell_get_object (plugin->shell,
			"IAnjutaDocumentManager", NULL /* TODO */);
	return IANJUTA_DOCUMENT_MANAGER (obj);
}


void
breakpoints_info (GList * outputs, gpointer data )
{
	BreakpointsDBase* bd = data;
	
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
	                           breakpoints_dbase_update, bd);
}

void
enable_all_breakpoints (BreakpointsDBase* bd)
{

#ifdef ANJUTA_DEBUG_DEBUGGER
	g_message ("In function: enable_all_breakpoints()");
#endif
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger_put_cmd_in_queqe ("enable breakpoints", DB_CMD_ALL,
	                           breakpoints_info, bd);
	gdb_util_append_message (bd->priv->plugin, _("All breakpoints enabled:"));
	debugger_execute_cmd_in_queqe ();
}

void
disable_all_breakpoints (BreakpointsDBase* bd)
{

#ifdef ANJUTA_DEBUG_DEBUGGER
	g_message ("In function: disable_all_breakpoints()");
#endif
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	debugger_put_cmd_in_queqe ("disable breakpoints", DB_CMD_ALL,
	                           breakpoints_info, bd);
	gdb_util_append_message (bd->priv->plugin, _("All breakpoints disabled:"));
	debugger_execute_cmd_in_queqe ();
}

void
delete_breakpoint (gint id, BreakpointsDBase* bd, gboolean end)
{
	gchar buff[20];
	gchar *cmd;
	static GList *list_bp= NULL;
	

#ifdef ANJUTA_DEBUG_DEBUGGER
	g_message ("In function: delete_breakpoint()");
#endif
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	
	if (!end)
	{
		list_bp = g_list_prepend (list_bp, GINT_TO_POINTER (id));
		return;
	}
	else
	{
		cmd = g_strdup("delete");
		if (id != 0)
		{
			sprintf (buff, " %d", id);
			cmd = g_strconcat(cmd, buff, NULL);
		}
		else
		{
			while (list_bp)
			{
				sprintf (buff, " %d", GPOINTER_TO_INT (list_bp->data));
				cmd = g_strconcat(cmd, buff, NULL);
				list_bp = list_bp->next;
			}
		}
	
		debugger_put_cmd_in_queqe (cmd, DB_CMD_ALL, breakpoints_info, bd);	
		debugger_execute_cmd_in_queqe ();
		
		g_free(cmd);
		g_list_free(list_bp);
	}
}

void
property_add_item (GList * outputs, gpointer data )
{
	Properties * prop = data;
	
	if (bk_item_add (prop->bd, prop->gxml, prop->bid) == TRUE)
			gtk_widget_destroy (prop->dialog);
	
	g_object_unref (prop->gxml);
}


void
property_destroy_breakpoint (BreakpointsDBase* bd, GladeXML *gxml,
	BreakpointItem *bid, GtkWidget *dialog)
{
	gchar buff[20];
	static Properties prop;

#ifdef ANJUTA_DEBUG_DEBUGGER
	g_message ("In function: delete_breakpoint()");
#endif
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	
	prop.bd = bd;
	prop.gxml = gxml;
	prop.bid = bid;
	prop.dialog = dialog;
	
	sprintf (buff, "delete %d", bid->id);
	
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, property_add_item, &prop);	
	debugger_execute_cmd_in_queqe ();
}


void
enable_breakpoint (gint id, BreakpointsDBase* bd)
{
	gchar buff[20];

#ifdef ANJUTA_DEBUG_DEBUGGER
	g_message ("In function: enable_breakpoint()");
#endif
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	sprintf (buff, "enable %d", id);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, breakpoints_info, bd);
	debugger_execute_cmd_in_queqe ();
}

void
disable_breakpoint (gint id, BreakpointsDBase* bd)
{
	gchar buff[20];

#ifdef ANJUTA_DEBUG_DEBUGGER
	g_message ("In function: disable_breakpoint()");
#endif
	
	if (debugger_is_active () == FALSE)
		return;
	if (debugger_is_ready () == FALSE)
		return;
	sprintf (buff, "disable %d", id);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL, breakpoints_info, bd);
	debugger_execute_cmd_in_queqe ();
}


static BreakpointItem *
breakpoint_item_new ()
{
	BreakpointItem *bi;
	bi = g_malloc (sizeof (BreakpointItem));

	bi->bd = NULL;
	bi->is_editing = FALSE;
	
	bi->id = 0;
	bi->enable = FALSE;
	bi->pass = 0;
	bi->condition = NULL;
	bi->file = NULL;
	bi->line = 0;
	bi->function = NULL;
	bi->handle = 0;
	bi->handle_invalid = TRUE;
	bi->info = NULL;
	bi->disp = NULL;
	return bi;
}

static void
breakpoint_item_destroy (BreakpointItem *bi)
{
	if (!bi)
		return;

	if (bi->function)
		g_free (bi->function);
	if (bi->file)
		g_free (bi->file);
	if (bi->condition)
		g_free (bi->condition);
	if (bi->info)
		g_free (bi->info);
	if (bi->disp)
		g_free (bi->disp);

	g_free (bi);
}

static gint
breakpoint_item_calc_size (BreakpointItem *bi)
{
	g_return_val_if_fail (bi != NULL, 0);
	
	return 

	+gdb_util_calc_gnum_len (/*bi->id*/)
	+gdb_util_calc_string_len (bi->disp)
	+gdb_util_calc_gnum_len (/*bi->enable*/)
	+gdb_util_calc_gnum_len (/*bi->addr*/)
	+gdb_util_calc_gnum_len (/*bi->pass*/)
	+gdb_util_calc_string_len (bi->condition)
	+gdb_util_calc_string_len (bi->file )

	+gdb_util_calc_gnum_len (/*bi->line*/)
	+gdb_util_calc_gnum_len (/*bi->handle*/)	
	+gdb_util_calc_gnum_len (/*bi->handle_invalid*/)	
	+gdb_util_calc_string_len (bi->function)
	+gdb_util_calc_string_len (bi->info)
	+gdb_util_calc_gnum_len (/*bi->time*/);
}

/* The saving format is a single string comma separated */
#if 0
static void
breakpoint_item_save (BreakpointItem *bi, ProjectDBase *pdb, const gint nBreak)
{
	gint	nSize ;
	gchar	*szStrSave, *szDst ;
	
	g_return_if_fail (bi != NULL);
	g_return_if_fail (pdb != NULL);

	nSize = breakpoint_item_calc_size (bi);
	szStrSave = g_malloc (nSize);
	if (NULL == szStrSave)
		return ;
	szDst = szStrSave ;
	/* Writes the fields to the string */
	szDst = WriteBufI (szDst, bi->id);
	szDst = WriteBufS (szDst, bi->disp);
	szDst = WriteBufB (szDst, bi->enable);
	szDst = WriteBufUL (szDst, bi->addr);
	szDst = WriteBufI (szDst, bi->pass);
	szDst = WriteBufS (szDst, bi->condition);
	szDst = WriteBufS (szDst, bi->file);
	szDst = WriteBufI (szDst, bi->line);
	szDst = WriteBufI (szDst, bi->handle);
	szDst = WriteBufB (szDst, bi->handle_invalid);
	szDst = WriteBufS (szDst, bi->function);
	szDst = WriteBufS (szDst, bi->info );	
	szDst = WriteBufUL (szDst, (gulong)bi->time);

	session_save_string_n (pdb, SECSTR(SECTION_BREAKPOINTS), nBreak, szStrSave);

	g_free (szStrSave);
};
#endif

#define	ASS_STR(x,nItem) \
	do \
	{ \
		if (NULL == ( bi->x = gdb_util_get_str_cod( p[nItem] ) ) ) \
		{ \
			goto fine; \
		} \
	} while(0)

static gboolean
breakpoint_item_load (BreakpointItem *bi, gchar *szStr)
{
	gchar		**p;
	gboolean	bOK = FALSE ;
	
	g_return_val_if_fail (bi != NULL, FALSE);
	
	p = PARSE_STR (BKPT_FIELDS,szStr);
	if (NULL == p)
		return FALSE ;
	bi->id	= atoi (p[0]);
	ASS_STR (disp, 1);
	bi->enable = atoi (p[2]) ? TRUE : FALSE ;
	bi->addr = atol (p[3]);
	bi->pass = atol (p[4]);
	ASS_STR (condition, 5);
	ASS_STR (file, 6);
	bi->line = atoi (p[7]);
	bi->handle = atoi (p[8]);
	bi->handle_invalid = atoi (p[9]) ? TRUE : FALSE ;
	ASS_STR (function, 10);
	ASS_STR (info, 11);
	bi->time = atol (p[12]);
	bOK = TRUE;
fine:
	g_free (p);
	return bOK;
};

static void
on_bk_remove_clicked (GtkWidget *button, gpointer data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean valid;

	bd = (BreakpointsDBase *) data;

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);		
		delete_breakpoint (bi->id, bd, TRUE);
	}
}

static void
on_bk_jumpto_clicked (GtkWidget *button, gpointer   data)
{
	BreakpointsDBase *bd;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean valid;
	IAnjutaDocumentManager *docman;

	bd = (BreakpointsDBase *) data;

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (valid)
	{
		BreakpointItem *bi;
		gtk_tree_model_get (model, &iter, DATA_COLUMN, &bi, -1);
		breakpoints_dbase_hide (bd);
		docman = get_document_manager (bd->priv->plugin);
		ianjuta_document_manager_goto_file_line (docman, bi->file, bi->line,
			NULL /* TODO */);
	}
}

/*************************************************************************************/

static void
pass_item_add_mesg_arrived (GList * lines, gpointer data)
{
	GList *outputs;
	BreakpointsDBase* bd = data;	
	
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
								breakpoints_dbase_update, bd);
	debugger_execute_cmd_in_queqe ();
	outputs = gdb_util_remove_blank_lines (lines);

	if (outputs == NULL)
		return;

	if (g_list_length (outputs) != 1)
	{
		gchar *msg, *tmp;
		gint i;

		msg = g_strdup ((gchar *) g_list_nth_data (outputs, 0));

		for (i = 1; i < g_list_length (outputs); i++)
		{
			tmp = msg;
			msg =
				g_strconcat (tmp, "\n",
					     (gchar *)
					     g_list_nth_data (outputs, i),
					     NULL);
			g_free (tmp);
		}
/* TODO		anjuta_information (msg); */
		g_free (msg);
		g_list_free (outputs);
		return;
	}

	if (strstr ((gchar *) outputs->data, "Will ignore next") == NULL) 
	{
/* TODO		anjuta_information((gchar *) g_list_nth_data (outputs, 0)); */
		g_list_free (outputs);
		return;
	}
}

static void
bk_item_add_mesg_arrived (GList * lines, gpointer data)
{
	BreakpointItem *bid;
	GList *outputs;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *editor = NULL;

	bid = (BreakpointItem*) data;
	outputs = gdb_util_remove_blank_lines (lines);
	if (outputs == NULL)
		goto down_label;
	if (g_list_length (outputs) == 1)
	{
		gint id, count, line;
		gchar file[256];
		gchar *buff;
		
		count =	sscanf ((char *) g_list_nth_data (outputs, 0),
			"Breakpoint %d at %*s file %s line %d", &id, file, &line);
		if (count != 3)
		{
			debugger_dialog_message (outputs, data);
			goto down_label;
		}
		/* Removing the extra comma at the end of the filename */
		if (strlen(file))
			file[strlen(file)-1] = '\0';

		docman = get_document_manager (bid->bd->priv->plugin);
		editor = ianjuta_document_manager_get_current_editor (docman, NULL /* TODO */);
		ianjuta_editor_goto_line (editor, line, NULL /* TODO */);
		
		if (bid == NULL)
			goto down_label;

		if (bid->pass > 0)
		{
			buff = g_strdup_printf ("ignore %d %d", id, bid->pass);
			debugger_put_cmd_in_queqe (buff, DB_CMD_ALL,
									   pass_item_add_mesg_arrived,
									   bid->bd);
			debugger_execute_cmd_in_queqe ();
			g_free (buff);
		}
	}
	else
	{
		debugger_dialog_message (outputs, data);
	}

down_label:
	debugger_put_cmd_in_queqe ("info breakpoints",
							   DB_CMD_NONE,
							   breakpoints_dbase_update,
							   bid->bd);
	debugger_execute_cmd_in_queqe ();
	if (outputs)
		g_list_free (outputs);

	return;
}

static gboolean
bk_item_add (BreakpointsDBase *bd, GladeXML *gxml, BreakpointItem *bid)
{
	GtkWidget *dialog;
	GtkWidget *location_entry;
	GtkWidget *condition_entry;
	GtkWidget *pass_entry;
	gchar *buff;

	dialog = glade_xml_get_widget (gxml, "breakpoints_properties_dialog");
	location_entry = glade_xml_get_widget (gxml, "breakpoint_location_entry");
	condition_entry = glade_xml_get_widget (gxml, "breakpoint_condition_entry");
	pass_entry = glade_xml_get_widget (gxml, "breakpoint_pass_entry");

	if (strlen (gtk_entry_get_text (GTK_ENTRY (location_entry))) > 0)
	{
		const gchar *loc_text;
		const gchar *cond_text;
		const gchar *pass_text;
	
		loc_text = gtk_entry_get_text (GTK_ENTRY (location_entry));
		cond_text = gtk_entry_get_text (GTK_ENTRY (condition_entry));
		pass_text = gtk_entry_get_text (GTK_ENTRY (pass_entry));
		
		bid->pass = atoi (pass_text);
		bid->condition = g_strdup (cond_text);
		bid->bd = bd;
		bid->is_editing = FALSE;
		if (strlen (cond_text) != 0)
			buff = g_strdup_printf ("break %s if %s", loc_text, cond_text);
		else
			buff = g_strdup_printf ("break %s", loc_text);
		
		debugger_put_cmd_in_queqe (buff,
								   DB_CMD_ALL,
								   bk_item_add_mesg_arrived,
								   bid);
		g_free (buff);
		debugger_execute_cmd_in_queqe ();
		return TRUE;
	}
	else
	{
/* TODO
		anjuta_error_parented (dialog, 
				_("You must give a valid location to set the breakpoint."));
*/
		debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
								   breakpoints_dbase_update, bd);
		debugger_execute_cmd_in_queqe ();
		return TRUE;
	}
}

static void
on_bk_properties_clicked (GtkWidget *button, gpointer   data)
{
	GladeXML *gxml;
	BreakpointsDBase *bd;
	BreakpointItem *bid;
	GtkWidget *dialog;
	GtkWidget *location_entry;
	GtkWidget *condition_entry;
	GtkWidget *pass_entry;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	gchar *buff;
	
	bd = (BreakpointsDBase *) data;
	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_selection_get_selected (selection, &model, &iter);
	if (!valid)
	{
		g_object_unref (gxml);
		return;
	}
	gtk_tree_model_get (model, &iter, DATA_COLUMN, &bid, -1);
	bid->bd = bd;
	bid->is_editing = TRUE;

	gxml = glade_xml_new (PREFS_GLADE,
						  "breakpoint_properties_dialog", NULL);
	dialog = glade_xml_get_widget (gxml, "breakpoint_properties_dialog");
	location_entry = glade_xml_get_widget (gxml, "breakpoint_location_entry");
	condition_entry = glade_xml_get_widget (gxml, "breakpoint_condition_entry");
	pass_entry = glade_xml_get_widget (gxml, "breakpoint_pass_entry");
	
	if (bid->file && strlen (bid->file) > 0)
		buff = g_strdup_printf ("%s:%d", bid->file, bid->line);
	else
		buff = g_strdup_printf ("%d", bid->line);
	gtk_entry_set_text (GTK_ENTRY (location_entry), buff);
	g_free (buff);
	if (bid->condition && strlen (bid->condition) > 0)
		gtk_entry_set_text (GTK_ENTRY (condition_entry), bid->condition);
	buff = g_strdup_printf ("%d", bid->pass);
	gtk_entry_set_text (GTK_ENTRY (pass_entry), buff);
	g_free (buff);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		property_destroy_breakpoint (bd, gxml, bid, dialog);
	}
	else
	{
		gtk_widget_destroy (dialog);
		g_object_unref (gxml);
	}
}

static void
on_bk_add_clicked (GtkWidget *button, gpointer data)
{
/* TODO
	GladeXML *gxml;
	BreakpointsDBase *bd;
	GtkWidget *dialog;
	BreakpointItem *bid;
	guint line;
	gchar *buff;
	TextEditor* te;
	GtkWidget *location_entry;
	
	bd = (BreakpointsDBase *) data;
	gxml = glade_xml_new (PREFS_GLADE,
						  "breakpoint_properties_dialog", NULL);
	dialog = glade_xml_get_widget (gxml, "breakpoint_properties_dialog");
	
	if((te = anjuta_get_current_text_editor())) 
	{
		if (te->filename)
		{
			if ((line = text_editor_get_current_lineno (te)))
				buff = g_strdup_printf("%s:%d", te->filename, line);
			else
				buff = g_strdup_printf("%s", te->filename);
			location_entry = glade_xml_get_widget (gxml, 
			                      "breakpoint_location_entry");
			gtk_entry_set_text (GTK_ENTRY (location_entry), buff);
			g_free (buff);
		}
	}
	bid = breakpoint_item_new ();
	bid->bd = bd;
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
	{
		if (bk_item_add (bd, gxml, bid) == TRUE)
			gtk_widget_destroy (dialog);
	}
	else
	{
		gtk_widget_destroy (dialog);
	}
	g_object_unref (gxml);
*/
}

static void
on_bk_removeall_clicked (GtkWidget *button, BreakpointsDBase *bd)
{
	GtkWidget *dialog;
	
	dialog = gtk_message_dialog_new (GTK_WINDOW (bd->priv->window),
									 GTK_DIALOG_MODAL |
										GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_NONE,
					_("Are you sure you want to delete all the breakpoints?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
							GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
							GTK_STOCK_DELETE, GTK_RESPONSE_YES,
							NULL);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES) {
		debugger_delete_all_breakpoints ();
		breakpoints_dbase_clear (bd);
	} else
		breakpoints_dbase_hide (bd);
	gtk_widget_destroy (dialog);
}

static void
on_bk_enableall_clicked (GtkWidget *button, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	BreakpointsDBase *bd = data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							ENABLED_COLUMN, TRUE, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
	}

#warning "G2 port: change the active state of the enabled column"
	 enable_all_breakpoints (bd);
}

static void
on_bk_disableall_clicked (GtkWidget *button, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	BreakpointsDBase *bd = data;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
							ENABLED_COLUMN, FALSE, -1);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
#warning "G2 port: change the active state of the enabled column"
	disable_all_breakpoints (bd);
}

static void
on_treeview_enabled_toggled (GtkCellRendererToggle *cell,
							 gchar			*path_str,
							 gpointer		 data)
{
	BreakpointsDBase *bd;
	BreakpointItem *bi;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean state;
	GtkTreePath *path;

	path = gtk_tree_path_new_from_string (path_str);

	bd = (BreakpointsDBase *) data;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state,
						DATA_COLUMN, &bi, -1);
	bi->enable ^= 1;

	if (!bi->enable)
		disable_breakpoint (bi->id, bd);
	else
		enable_breakpoint (bi->id, bd);

	state ^= 1;

	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, ENABLED_COLUMN,
						state, -1);
}

static void
on_bk_treeview_selection_changed (GtkTreeSelection *selectoin,
								  BreakpointsDBase *bd)
{
	breakpoints_dbase_update_controls (bd);
}


static gboolean
on_bk_window_delete_event (GtkWindow *win, GdkEvent *event, BreakpointsDBase *bd)
{
	breakpoints_dbase_hide (bd);
	return TRUE;
}

BreakpointsDBase *
breakpoints_dbase_new (AnjutaPlugin *plugin)
{
	BreakpointsDBase *bd;
	
	bd = g_new0 (BreakpointsDBase, 1);
	bd->priv = g_new0 (BreakpointsDBasePriv, 1);
	if (bd) {
		GtkTreeView *view;
		GtkTreeStore *store;
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		int i;

		bd->priv->plugin = plugin;

		/* breakpoints dialog */
		bd->priv->gxml = glade_xml_new (PREFS_GLADE,
										"breakpoints_dialog", NULL);
		glade_xml_signal_autoconnect (bd->priv->gxml);
		
		bd->priv->window = glade_xml_get_widget (bd->priv->gxml,
												 "breakpoints_dialog");
		gtk_widget_hide (bd->priv->window);
/* TODO
		gtk_window_set_transient_for (GTK_WINDOW (bd->priv->window),
									  GTK_WINDOW (app));
*/
		bd->priv->treeview = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_tv");
		bd->priv->remove_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_remove_button");
		bd->priv->jumpto_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_jumpto_button");
		bd->priv->properties_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_properties_button");
		bd->priv->add_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_add_button");
		bd->priv->removeall_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_removeall_button");
		bd->priv->enableall_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_enableall_button");
		bd->priv->disableall_button = glade_xml_get_widget (bd->priv->gxml,
												"breakpoints_disableall_button");

		view = GTK_TREE_VIEW (bd->priv->treeview);
		store = gtk_tree_store_new (COLUMNS_NB,
									G_TYPE_BOOLEAN,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_STRING,
									G_TYPE_POINTER);
		gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
		gtk_tree_selection_set_mode (gtk_tree_view_get_selection (view),
					     GTK_SELECTION_SINGLE);
		g_object_unref (G_OBJECT (store));

		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (column_names[0],
														   renderer,
								   						   "active",
														   ENABLED_COLUMN,
														   NULL);
		g_signal_connect (renderer, "toggled",
						  G_CALLBACK (on_treeview_enabled_toggled), bd);
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_append_column (view, column);
		renderer = gtk_cell_renderer_text_new ();

		for (i = FILENAME_COLUMN; i < (COLUMNS_NB - 1); i++) {
			column =
				gtk_tree_view_column_new_with_attributes (column_names[i],
													renderer, "text", i, NULL);
			gtk_tree_view_column_set_sizing (column,
											 GTK_TREE_VIEW_COLUMN_AUTOSIZE);
			gtk_tree_view_append_column (view, column);
		}

		g_signal_connect (G_OBJECT (bd->priv->remove_button), "clicked",
				  G_CALLBACK (on_bk_remove_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->jumpto_button), "clicked",
				  G_CALLBACK (on_bk_jumpto_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->properties_button), "clicked",
				  G_CALLBACK (on_bk_properties_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->add_button), "clicked",
				  G_CALLBACK (on_bk_add_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->removeall_button), "clicked",
				  G_CALLBACK (on_bk_removeall_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->enableall_button), "clicked",
				  G_CALLBACK (on_bk_enableall_clicked), bd);
		g_signal_connect (G_OBJECT (bd->priv->disableall_button), "clicked",
				  G_CALLBACK (on_bk_disableall_clicked), bd);

		g_signal_connect (G_OBJECT (gtk_tree_view_get_selection
					(GTK_TREE_VIEW (bd->priv->treeview))), "changed",
						  G_CALLBACK (on_bk_treeview_selection_changed), bd);
		g_signal_connect (G_OBJECT (bd->priv->window), "delete_event",
				  G_CALLBACK (on_bk_window_delete_event), bd);

		bd->priv->cond_history = NULL;
		bd->priv->pass_history = NULL;
		bd->priv->loc_history = NULL;
		bd->priv->is_showing = FALSE;
		bd->priv->win_pos_x = 50;
		bd->priv->win_pos_y = 50;
		bd->priv->win_width = 500;
		bd->priv->win_height = 300;
	}

	return bd;
}

void
breakpoints_dbase_destroy (BreakpointsDBase * bd)
{
	g_return_if_fail (bd != NULL);
	if (bd->priv->cond_history)
		g_free (bd->priv->cond_history);
	if (bd->priv->pass_history)
		g_free (bd->priv->pass_history);
	if (bd->priv->loc_history)
		g_free (bd->priv->loc_history);

	gtk_widget_destroy (bd->priv->window);
	g_object_unref (bd->priv->gxml);
	g_free (bd->priv);
	g_free (bd);
}

/* FIXME: Dont' make it static */
static int count;

static gboolean
on_treeview_foreach (GtkTreeModel *model, GtkTreePath *path,
					 GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *bi;
/* TODO	ProjectDBase *pdb = data; */
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
/* TODO	breakpoint_item_save (bi, pdb, count); */
	count++;
	return FALSE;
}

/* TODO
void
breakpoints_dbase_save (BreakpointsDBase * bd, ProjectDBase * pdb )
{
	GtkTreeModel *model;

	g_return_if_fail (bd != NULL);
	g_return_if_fail (pdb != NULL);
	
	session_clear_section( pdb, SECSTR(SECTION_BREAKPOINTS) );
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	count = 0;
	gtk_tree_model_foreach (model, on_treeview_foreach, pdb);
}

void
breakpoints_dbase_load (BreakpointsDBase *bd, ProjectDBase *p)
{
	gpointer config_iterator;
	guint loaded = 0;

	g_return_if_fail (p != NULL);
	breakpoints_dbase_clear(bd);
	config_iterator = session_get_iterator (p, SECSTR (SECTION_BREAKPOINTS));
	if (config_iterator !=  NULL)
	{
		gchar *szItem, *szData;
		loaded = 0;
		while ((config_iterator = gnome_config_iterator_next (config_iterator,
															  &szItem,
															  &szData )))
		{
			// shouldn't happen, but I'm paranoid
			if( NULL != szData )
			{
				BreakpointItem *bi;
				gboolean bToDel = TRUE ;
				
				bi = breakpoint_item_new ();
				if (NULL != bi)
				{
					if (breakpoint_item_load (bi, szData))
					{
						breakpoints_dbase_set_from_item (bd, bi, TRUE);
						bToDel = FALSE ;
					}
				}
				if (bToDel && bi)
					breakpoint_item_destroy (bi);
			}
			loaded++;
			g_free (szItem);
			g_free (szData);
		}
	}
}
*/

#if 0
void
experimental_not_use_breakpoints_dbase_toggle_breakpoint (BreakpointsDBase* b)
{
	guint line;
	struct BkItemData *bid;
	gchar *buff;
	TextEditor* te;

	g_return_if_fail (b != NULL);
	te = anjuta_get_current_text_editor();
	g_return_if_fail (te != NULL);
	
	if (debugger_is_active()==FALSE) return;
	if (debugger_is_ready()==FALSE) return;

	line = text_editor_get_current_lineno (te);
	/* Is breakpoint set? */
	
	/* Brakpoint is not set. So, set it. */
	bid = g_malloc (sizeof(struct BkItemData));
	if (bid == NULL) return;
	bid->loc_text = g_strdup_printf ("%s:%d", te->filename, line);
	bid->cond_text = g_strdup("");
	bid->pass_text = g_strdup("");
	bid->bd = b;

	buff = g_strdup_printf ("break %s", bid->loc_text);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL,
				   bk_item_add_mesg_arrived,
				   bid);
	g_free (buff);
	debugger_execute_cmd_in_queqe ();
}
#endif

void
breakpoints_dbase_clear (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);

	breakpoints_dbase_delete_all_breakpoints (bd);
	if (bd->priv->treeview) {
		GtkTreeModel *model =
			gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
		gtk_tree_store_clear (GTK_TREE_STORE (model));
	}

	breakpoints_dbase_delete_all_markers (bd, IANJUTA_MARKABLE_LIGHT);
	breakpoints_dbase_delete_all_markers (bd, IANJUTA_MARKABLE_INTENSE);
}

static void
breakpoints_dbase_delete_all_breakpoints (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);

	/* TODO */
}

static void
breakpoints_dbase_delete_all_markers (BreakpointsDBase *bd,
		IAnjutaMarkableMarker marker)
{
	IAnjutaDocumentManager *docman = get_document_manager (bd->priv->plugin);
	GList *editors, *node;

	g_return_if_fail (docman != NULL);

	editors = ianjuta_document_manager_get_editors (docman, NULL /* TODO */);
	if (editors)
	{
		node = editors;
		do
		{
			if (IANJUTA_IS_MARKABLE (node))
			{
				ianjuta_markable_delete_all_markers (IANJUTA_MARKABLE (node),
						marker, NULL /* TODO */);
			}
			node = g_list_next (node);
		}
		while (node);
		g_list_free (editors);
	}
}

void
breakpoints_dbase_show (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);
	if (bd->priv->is_showing)
	{
		gdk_window_raise (bd->priv->window->window);
		return;
	}
	gtk_widget_set_uposition (bd->priv->window,
							  bd->priv->win_pos_x,
							  bd->priv->win_pos_y);
	gtk_window_set_default_size (GTK_WINDOW
								 (bd->priv->window),
								 bd->priv->win_width,
								 bd->priv->win_height);
	gtk_widget_show (bd->priv->window);
	bd->priv->is_showing = TRUE;
}

void
breakpoints_dbase_hide (BreakpointsDBase *bd)
{
	g_return_if_fail (bd != NULL);
	if (bd->priv->is_showing == FALSE)
		return;
	gdk_window_get_root_origin (bd->priv->window->
								window, &bd->priv->win_pos_x,
								&bd->priv->win_pos_y);
	gdk_window_get_size (bd->priv->window->window,
						 &bd->priv->win_width, &bd->priv->win_height);
	gtk_widget_hide (bd->priv->window);
	bd->priv->is_showing = FALSE;
}

/* call back for the gdb 'info breakpoints' command'
   parse the gdb output and updates the db accordingly */
void
breakpoints_dbase_update (GList * outputs, gpointer data)
{
	BreakpointsDBase *bd;
	gchar *ptr;
	GList *list, *node;

	bd = (BreakpointsDBase *) data;

	list = gdb_util_remove_blank_lines (outputs);
	breakpoints_dbase_clear (bd);
	if (g_list_length (list) < 2)
	{
		g_list_free (list);
		return;
	}
	if (!strcmp ((gchar *) list->data, "No breakpoints or watchpoints"))
	{
		g_list_free (list);
		return;
	}

	ptr = g_strconcat ((gchar *) list->next->data, "\n", NULL);
	node = list->next->next;
	while (node)
	{
		gchar *line = (gchar *) node->data;
		node = node->next;
		if (isspace (line[0]))	/* line break */
		{
			gchar *tmp;
			tmp = ptr;
			ptr = g_strconcat (tmp, line, "\n", NULL);
			g_free (tmp);
		}
		else
		{
			breakpoints_dbase_add_brkpnt (bd, ptr);
			g_free (ptr);
			ptr = g_strconcat (line, "\n", NULL);
		}
	}
	if (ptr)
	{
		breakpoints_dbase_add_brkpnt (bd, ptr);
		g_free (ptr);
		ptr = NULL;
	}
	breakpoints_dbase_update_controls (bd);
	g_list_free (list);
}

static gboolean
breakpoints_dbase_set_from_item (BreakpointsDBase *bd, BreakpointItem *bi,
								 gboolean add_to_treeview)
{
	struct stat st;
	const gchar *fn;
	gchar *buff;
	gboolean disable, old;
	gint ret;
	IAnjutaDocumentManager *docman;
	
	old = disable = FALSE;
	docman = get_document_manager (bd->priv->plugin);
	fn = ianjuta_document_manager_get_full_filename (docman, bi->file, NULL /* TODO */);
	ret = stat (fn, &st);
	if (ret != 0)
	{
		old = TRUE;
		disable = TRUE;
	}
	if (bi->time < st.st_mtime)
	{
		old = TRUE;
		disable = TRUE;
	}
	if (bi->condition)
	{
		buff =
			g_strdup_printf ("break %s:%u if %s", bi->file, bi->line,
							 bi->condition);
		debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL, NULL);
		g_free (buff);
	}
	else
	{
		buff =
			g_strdup_printf ("break %s:%u", bi->file, bi->line);
		debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL, NULL);
		g_free (buff);
	}
	if (bi->pass > 0)
	{
		buff = g_strdup_printf ("ignore $bpnum %d", bi->pass);
		debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL, NULL);
		g_free (buff);
	}
	if (disable)
	{
		buff = g_strdup_printf ("disable $bpnum");
		debugger_put_cmd_in_queqe (buff, DB_CMD_NONE, NULL, NULL);
		g_free (buff);
		disable = FALSE;
	}
	return old;
}

static gboolean
on_set_breakpoint_foreach (GtkTreeModel *model, GtkTreePath *path,
						   GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *bi;
	BreakpointsDBase *bd = data;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
	breakpoints_dbase_set_from_item (bd, bi, FALSE);
	return FALSE;
}

void
breakpoints_dbase_set_all (BreakpointsDBase * bd)
{
	gboolean old = FALSE;
	GtkTreeModel *model;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_foreach (model, on_set_breakpoint_foreach, bd);

	if (old)
	{
/* TODO		anjuta_warning (_("Old breakpoints disabled.")); */
	}
	debugger_put_cmd_in_queqe ("info breakpoints", DB_CMD_NONE,
							   breakpoints_dbase_update,
							   bd);
}

gboolean
breakpoints_dbase_save_yourself (BreakpointsDBase * bd, FILE *stream)
{
	g_return_val_if_fail (bd != NULL, FALSE);

	if (bd->priv->is_showing)
	{
		gdk_window_get_root_origin (bd->priv->window->window,
					&bd->priv->win_pos_x, &bd->priv->win_pos_y);
		gdk_window_get_size (bd->priv->window->window,
					&bd->priv->win_width, &bd->priv->win_height);
	}
	fprintf (stream, "breakpoints.win.pos.x=%d\n", bd->priv->win_pos_x);
	fprintf (stream, "breakpoints.win.pos.y=%d\n", bd->priv->win_pos_y);
	fprintf (stream, "breakpoints.win.width=%d\n", bd->priv->win_width);
	fprintf (stream, "breakpoints.win.height=%d\n", bd->priv->win_height);
	return TRUE;
}

/* TODO
gboolean
breakpoints_dbase_load_yourself (BreakpointsDBase * bd, PropsID props)
{
	g_return_val_if_fail (bd != NULL, FALSE);
	bd->priv->win_pos_x = prop_get_int (props, "breakpoints.win.pos.x", 50);
	bd->priv->win_pos_y = prop_get_int (props, "breakpoints.win.pos.y", 50);
	bd->priv->win_width = prop_get_int (props, "breakpoints.win.width", 500);
	bd->priv->win_height = prop_get_int (props, "breakpoints.win.height", 300);
	return TRUE;
}
*/


/**********************************
 * Private functions: Do not use  *
 **********************************/
void
breakpoints_dbase_update_controls (BreakpointsDBase * bd)
{
	gboolean A, R, C, S;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	
	A = debugger_is_active ();
	R = debugger_is_ready ();

	selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW (bd->priv->treeview));
	S = gtk_tree_selection_get_selected (selection, &model, &iter);
	C = gtk_tree_model_get_iter_first(model, &iter);
	
	gtk_widget_set_sensitive (bd->priv->remove_button, A && R && C && S);
	gtk_widget_set_sensitive (bd->priv->jumpto_button, C && S);
	gtk_widget_set_sensitive (bd->priv->properties_button, A && R && C && S);

	gtk_widget_set_sensitive (bd->priv->add_button, A && R);
	gtk_widget_set_sensitive (bd->priv->removeall_button, A && R);
	gtk_widget_set_sensitive (bd->priv->enableall_button, A && R);
	gtk_widget_set_sensitive (bd->priv->disableall_button, A && R);
	gtk_widget_set_sensitive (bd->priv->treeview, A && R);
}

static void
breakpoints_dbase_add_brkpnt (BreakpointsDBase *bd, gchar *brkpnt)
{
	BreakpointItem *bi;
	GList *node;
	const gchar* full_fname = NULL;
	gchar brkno[10];
	gchar *function;
	gchar *fileln;
	gchar file[512];
	gchar line[10];
	gchar ignore[10];
	gchar enb[5];
	gchar cond[512];
	gchar *ptr;
	glong count;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *ed = NULL;
	IAnjutaMarkable *markable = NULL;

	g_return_if_fail (bd != NULL);

	/* this should not happen, abort */
	if (isspace (brkpnt[0]))
		return;

	/* only breakpoints, no watchpoints */
	if (strstr (brkpnt, "watchpoint"))
		return;

	count = sscanf (brkpnt, "%s %*s %*s %s", brkno, enb);
	function = strstr (brkpnt, " in ");
	fileln = g_strrstr (brkpnt, " at ");

	if (count == 2 && function && fileln)
	{
		GtkTreeStore *store;
		GtkTreeIter iter;

		if (count == 2)
		{
			function += 4; // skip " in "
			*fileln = 0; // finish function part
			fileln += 4; // skip " at "

			/* get file and line no */
			ptr = strchr (fileln, ':');
			ptr++;
			strcpy (line, ptr);
			ptr--;
			*ptr = '\0';
			strcpy (file, fileln);
		}

		/* add breakpoint to list */

		bi = breakpoint_item_new ();
		bi->file = g_strdup (file);

		bi->line = atoi (line);

		bi->function = g_strdup (function);
		bi->id = atoi (brkno);

		if (strcmp (enb, "y") == 0)
			bi->enable = TRUE;
		else
			bi->enable = FALSE;

		if ((ptr = strstr (brkpnt, "ignore")))
		{
			sscanf (ptr, "ignore next %s", ignore);
			bi->pass = atoi (ignore);
		}
		else
		{
			strcpy (ignore, _("0"));
			bi->pass = 0;
		}
		if ((ptr = strstr (brkpnt, "stop only if ")))
		{
			gint i = 0;
			ptr += strlen ("stop only if ");
			while (*ptr != '\n' && *ptr != '\0')
			{
				cond[i] = *ptr++;
				i++;
			}
			cond[i] = '\0';
			bi->condition = g_strdup (cond);
		}
		else
		{
			strcpy (cond, "");
			bi->condition = NULL;
		}

		docman = get_document_manager (bd->priv->plugin);
		full_fname = ianjuta_document_manager_get_full_filename (docman,
				bi->file, NULL /* TODO */);
		ed = ianjuta_document_manager_find_editor_with_path (docman, full_fname,
				NULL /* TODO */);
		if (ed != NULL && IANJUTA_IS_MARKABLE (ed))
		{
			/* IANJUTA_MARKABLE_LIGHT .......... normal breakpoint */
			/* IANJUTA_MARKABLE_INTENSE ........ disabled breakpoint */

			markable = IANJUTA_MARKABLE (ed);
			if (! ianjuta_markable_is_marker_set (markable, bi->line,
					IANJUTA_MARKABLE_LIGHT, NULL /* TODO */))
			{
				if (bi->enable)
				{
					if (ianjuta_markable_is_marker_set (markable, bi->line,
						IANJUTA_MARKABLE_INTENSE, NULL /* TODO */))
					{
						ianjuta_markable_unmark (markable, bi->line,
								IANJUTA_MARKABLE_INTENSE, NULL /* TODO */);
					}

					bi->handle = ianjuta_markable_mark (markable, bi->line,
							IANJUTA_MARKABLE_LIGHT, NULL /* TODO */);
				}
				else
				{
					if (! ianjuta_markable_is_marker_set (markable, bi->line,
						IANJUTA_MARKABLE_INTENSE, NULL /* TODO */))
					{
						bi->handle = ianjuta_markable_mark (markable, bi->line,
							IANJUTA_MARKABLE_INTENSE, NULL /* TODO */);
					}
				}
			}
			bi->handle_invalid = FALSE;
		}

		bi->time = time (NULL);

		store = GTK_TREE_STORE (gtk_tree_view_get_model
								(GTK_TREE_VIEW (bd->priv->treeview)));
		gtk_tree_store_append (store, &iter, NULL);
		
		gtk_tree_store_set (store, &iter,
							ENABLED_COLUMN, bi->enable,
							FILENAME_COLUMN, file,
							LINENO_COLUMN, line,
							FUNCTION_COLUMN, function,
							PASS_COLUMN, ignore,
							CONDITION_COLUMN, cond,
							DATA_COLUMN, bi,
							-1);
	}
}

/* TODO
static gboolean
on_set_breakpoint_te_foreach (GtkTreeModel *model, GtkTreePath *path,
							  GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *bi;
	TextEditor *te = data;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
	if (bi->line < 0 || bi->handle_invalid == FALSE)
		return FALSE;
	
	if (strcmp (te->filename, bi->file) == 0)
	{
		bi->handle_invalid = FALSE;
	}
	return FALSE;
}

void
breakpoints_dbase_set_all_in_editor (BreakpointsDBase* bd, TextEditor* te)
{
	GtkTreeModel *model;
	g_return_if_fail (te != NULL);
	g_return_if_fail (bd != NULL);
	if (te->full_filename == NULL)
		return;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_foreach (model, on_set_breakpoint_te_foreach, te);
}


static gboolean
on_clear_breakpoint_te_foreach (GtkTreeModel *model, GtkTreePath *path,
								GtkTreeIter *iter, gpointer data)
{
	gchar* full_fname;
	BreakpointItem *bi;
	TextEditor *te = data;
	
	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &bi, -1);
	if (bi->handle_invalid == TRUE)
		return FALSE;
	
	full_fname = anjuta_get_full_filename (bi->file);
	if (strcmp (te->full_filename, full_fname) == 0)
	{
		bi->handle_invalid = TRUE;
	}
	if (full_fname) g_free (full_fname);
	return FALSE;
}

void
breakpoints_dbase_clear_all_in_editor (BreakpointsDBase *bd, TextEditor *te)
{
	GtkTreeModel *model;
	g_return_if_fail (te != NULL);
	g_return_if_fail (bd != NULL);
	if (te->full_filename == NULL)
		return;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	gtk_tree_model_foreach (model, on_clear_breakpoint_te_foreach, te);
}

//gboolean flag;
*/

static gboolean
on_delete_matching_foreach (GtkTreeModel *model, GtkTreePath *path,
							GtkTreeIter *iter, gpointer data)
{
	BreakpointItem *item;
	gint moved_line, line;
	BreakpointsDBase* bd;
	IAnjutaDocumentManager *docman;
	IAnjutaEditor *te;
	const gchar *filename;

	bd = (BreakpointsDBase *) data;

	docman = get_document_manager (bd->priv->plugin);
	te = ianjuta_document_manager_get_current_editor (docman, NULL /* TODO */);

	gtk_tree_model_get (GTK_TREE_MODEL (model), iter, DATA_COLUMN, &item, -1);
	if (item->handle_invalid)
		return FALSE;

	filename = ianjuta_editor_get_filename (te, NULL /* TODO */);
	if (strcmp (filename, item->file) != 0)
		return FALSE;
	line = ianjuta_editor_get_lineno (te, NULL /* TODO */);
	if (IANJUTA_IS_MARKABLE (te))
	{
		moved_line = ianjuta_markable_location_from_handle (IANJUTA_MARKABLE (te),
				item->handle, NULL /* TODO */);
	}
	//if (moved_line == line && moved_line >= 0)
	if (item->line == line && item->line >= 0)
	{
		// delete breakpoint marker from screen
		if (IANJUTA_IS_MARKABLE (te))
		{
			ianjuta_markable_unmark (IANJUTA_MARKABLE (te), line,
					IANJUTA_MARKABLE_LIGHT, NULL /* TODO */);
		}
		// delete breakpoint in debugger
		delete_breakpoint (item->id, bd, FALSE);
	}
	return FALSE;
}


/*  if l == 0  :  line = current line */
/*  return FALSE if debugger not active */
gboolean
breakpoints_dbase_toggle_breakpoint (BreakpointsDBase *bd, guint l)
{
	guint line;
	BreakpointItem *bid;
	gchar *buff;
	IAnjutaDocumentManager *docman = NULL;
	IAnjutaEditor *te = NULL;
	IAnjutaMarkable *markable = NULL;
	const gchar *filename = NULL;

	if (debugger_is_active() == FALSE) return FALSE;
	if (debugger_is_ready() == FALSE) return FALSE;

	g_return_val_if_fail (bd != NULL, FALSE);

	docman = get_document_manager (bd->priv->plugin);
	g_return_val_if_fail (docman != NULL, FALSE);
	te = ianjuta_document_manager_get_current_editor (docman, NULL /* TODO */);
	g_return_val_if_fail (te != NULL, FALSE);
	
	if (l == 0)
		line = ianjuta_editor_get_lineno (te, NULL /* TODO */);
	else
	{
		line = l;
		ianjuta_editor_goto_line (te, line, NULL /* TODO */);
	}

	/* Is breakpoint set? */
	if (IANJUTA_IS_MARKABLE (te))
	{
		markable = IANJUTA_MARKABLE (te);
		if (ianjuta_markable_is_marker_set (markable, line,
				IANJUTA_MARKABLE_LIGHT, NULL /* TODO */) ||
			ianjuta_markable_is_marker_set (markable, line,
				IANJUTA_MARKABLE_INTENSE, NULL /* TODO */))
		{
			/* Breakpoint is set. So, delete it. */
			GtkTreeModel *model;
			
			model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
			
			gtk_tree_model_foreach (model, on_delete_matching_foreach, bd);
			delete_breakpoint (0, bd, TRUE);
			return TRUE;
		}
	}

	/* Breakpoint is not set. So, set it. */
	bid = breakpoint_item_new ();
	bid->pass = 0;
	bid->bd = bd;

	filename = ianjuta_editor_get_filename (te, NULL /* TODO */);
	buff = g_strdup_printf ("break %s:%d", filename, line);
	debugger_put_cmd_in_queqe (buff, DB_CMD_ALL,
							   bk_item_add_mesg_arrived,
							   bid);
	g_free (buff);
	debugger_execute_cmd_in_queqe ();
	return TRUE;
}


/*  return FALSE if debugger not active */
gboolean
breakpoints_dbase_toggle_doubleclick (guint line)
{
	return breakpoints_dbase_toggle_breakpoint(debugger.breakpoints_dbase, line);
}

static void
breakpoints_toggle_enable (BreakpointsDBase *bd, guint line)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	BreakpointItem *bi;
	char *treeline;
	gboolean state;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (bd->priv->treeview));
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		gtk_tree_model_get (model, &iter, ENABLED_COLUMN, &state,
						LINENO_COLUMN, &treeline, DATA_COLUMN, &bi, -1);
		if (atoi(treeline) == line)
		{
			bi->enable ^= 1;

			if (!bi->enable)
				disable_breakpoint (bi->id, bd);
			else
				enable_breakpoint (bi->id, bd);
		}
		valid = gtk_tree_model_iter_next (model, &iter);
	}
}


void
breakpoints_dbase_toggle_singleclick (guint line)
{
	breakpoints_toggle_enable (debugger.breakpoints_dbase, line);
}

void breakpoints_dbase_add (BreakpointsDBase *bd)
{
	on_bk_add_clicked (NULL, bd);
}

void breakpoints_dbase_disable_all (BreakpointsDBase *bd)
{
	on_bk_disableall_clicked (NULL, bd);
}

void breakpoints_dbase_remove_all (BreakpointsDBase *bd)
{
	on_bk_removeall_clicked (NULL, bd);
}
