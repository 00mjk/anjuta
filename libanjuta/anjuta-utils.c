/*
    anjuta-utils.c
    Copyright (C) 2003 Naba Kumar  <naba@gnome.org>

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

#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <glib/gi18n.h>

#include <libgnome/gnome-util.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#include <libgnomevfs/gnome-vfs.h>

#define FILE_BUFFER_SIZE 1024

gboolean
anjuta_util_copy_file (gchar * src, gchar * dest, gboolean show_error)
{
	FILE *input_fp, *output_fp;
	gchar buffer[FILE_BUFFER_SIZE];
	gint bytes_read, bytes_written;
	gboolean error;
	
	error = TRUE;
	
	input_fp = fopen (src, "rb");
	if (input_fp == NULL)
	{
		if( show_error)
			anjuta_util_dialog_error_system (NULL, errno,
											 _("Unable to read file: %s."),
											 src);
		return FALSE;
	}
	
	output_fp = fopen (dest, "wb");
	if (output_fp == NULL)
	{
		if( show_error)
			anjuta_util_dialog_error_system (NULL, errno,
											 _("Unable to create file: %s."),
											 dest);
		fclose (input_fp);
		return TRUE;
	}
	
	for (;;)
	{
		bytes_read = fread (buffer, 1, FILE_BUFFER_SIZE, input_fp);
		if (bytes_read != FILE_BUFFER_SIZE && ferror (input_fp))
		{
			error = FALSE;
			break;
		}
		
		if (bytes_read)
		{
			bytes_written = fwrite (buffer, 1, bytes_read, output_fp);
			if (bytes_read != bytes_written)
			{
				error = FALSE;
				break;
			}
		}
		
		if (bytes_read != FILE_BUFFER_SIZE && feof (input_fp))
		{
			break;
		}
	}
	
	fclose (input_fp);
	fclose (output_fp);
	
	if( show_error && (error == FALSE))
		anjuta_util_dialog_error_system (NULL, errno,
										 _("Unable to complete file copy"));
	return error;
}

static gint
int_from_hex_digit (const gchar ch)
{
	if (isdigit (ch))
		return ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else
		return 0;
}

void
anjuta_util_color_from_string (const gchar * val, guint8 * r, guint8 * g, guint8 * b)
{
	*r = int_from_hex_digit (val[1]) * 16 + int_from_hex_digit (val[2]);
	*g = int_from_hex_digit (val[3]) * 16 + int_from_hex_digit (val[4]);
	*b = int_from_hex_digit (val[5]) * 16 + int_from_hex_digit (val[6]);
}

gchar *
anjuta_util_string_from_color (guint8 r, guint8 g, guint8 b)
{
	gchar str[10];
	guint32 num;

	num = r;
	num <<= 8;
	num += g;
	num <<= 8;
	num += b;

	sprintf (str, "#%06X", num);
	return g_strdup (str);
}

GtkWidget* 
anjuta_util_button_new_with_stock_image (const gchar* text,
										 const gchar* stock_id)
{
	GtkWidget *button;
	GtkStockItem item;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *hbox;
	GtkWidget *align;

	button = gtk_button_new ();

 	if (GTK_BIN (button)->child)
    		gtk_container_remove (GTK_CONTAINER (button),
				      GTK_BIN (button)->child);

  	if (gtk_stock_lookup (stock_id, &item))
    	{
      		label = gtk_label_new_with_mnemonic (text);

		gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
      
		image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
      		hbox = gtk_hbox_new (FALSE, 2);

      		align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      
      		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      		gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      
      		gtk_container_add (GTK_CONTAINER (button), align);
      		gtk_container_add (GTK_CONTAINER (align), hbox);
      		gtk_widget_show_all (align);

      		return button;
    	}

      	label = gtk_label_new_with_mnemonic (text);
      	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
  
  	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  	gtk_widget_show (label);
  	gtk_container_add (GTK_CONTAINER (button), label);

	return button;
}

GtkWidget*
anjuta_util_dialog_add_button (GtkDialog *dialog, const gchar* text,
							   const gchar* stock_id, gint response_id)
{
	GtkWidget *button;
	
	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (stock_id != NULL, NULL);

	button = anjuta_util_button_new_with_stock_image (text, stock_id);
	g_return_val_if_fail (button != NULL, NULL);

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (dialog, button, response_id);	

	return button;
}

void
anjuta_util_dialog_error (GtkWindow *parent, const gchar *mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR,
									 GTK_BUTTONS_CLOSE, message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_warning (GtkWindow *parent, const gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_WARNING,
									 GTK_BUTTONS_CLOSE, message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_info (GtkWindow *parent, const gchar * mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_INFO,
									 GTK_BUTTONS_CLOSE, message);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (message);
}

void
anjuta_util_dialog_error_system (GtkWindow* parent, gint errnum,
								 const gchar * mesg, ... )
{
	gchar* message;
	gchar* tot_mesg;
	va_list args;
	GtkWidget *dialog;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);

	if (0 != errnum) {
		tot_mesg = g_strconcat (message, _("\nSystem: "),
								g_strerror(errnum), NULL);
		g_free (message);
	} else
		tot_mesg = message;

	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	// Dialog to be HIG compliant
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_ERROR,
									 GTK_BUTTONS_CLOSE, tot_mesg);
	g_signal_connect (G_OBJECT (dialog), "response",
					  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (tot_mesg);
}

gboolean
anjuta_util_dialog_boolean_question (GtkWindow *parent, const gchar *mesg, ...)
{
	gchar* message;
	va_list args;
	GtkWidget *dialog;
	gint ret;
	GtkWindow *real_parent;

	va_start (args, mesg);
	message = g_strdup_vprintf (mesg, args);
	va_end (args);
	
	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}
	
	dialog = gtk_message_dialog_new (real_parent,
									 GTK_DIALOG_DESTROY_WITH_PARENT,
									 GTK_MESSAGE_QUESTION,
									 GTK_BUTTONS_YES_NO, message);

	ret = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (message);
	
	return (ret == GTK_RESPONSE_YES);
}

gboolean
anjuta_util_dialog_input (GtkWindow *parent, const gchar *prompt,
						  const gchar *default_value, gchar **return_value)
{
	GtkWidget *dialog, *label, *frame, *entry, *dialog_vbox, *vbox;
	gint res;
	gchar *markup;
	GtkWindow *real_parent;

	if (parent && GTK_IS_WINDOW (parent))
	{
		real_parent = parent;
	}
	else
	{
		real_parent = NULL;
	}

	dialog = gtk_dialog_new_with_buttons (prompt, real_parent,
										  GTK_DIALOG_DESTROY_WITH_PARENT,
										  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										  GTK_STOCK_OK, GTK_RESPONSE_OK,
										  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	dialog_vbox = GTK_DIALOG (dialog)->vbox;
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, -1);
	gtk_widget_show (dialog_vbox);
	
	markup = g_strconcat ("<b>", prompt, "</b>", NULL);
	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	gtk_widget_show (label);
	g_free (markup);
	
	frame = gtk_frame_new (NULL);
	gtk_frame_set_label_widget (GTK_FRAME (frame), label);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 10);
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (dialog_vbox), frame, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	entry = gtk_entry_new ();
	gtk_widget_show (entry);
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
	if (default_value)
		gtk_entry_set_text (GTK_ENTRY (entry), default_value);
	
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	
	if (gtk_entry_get_text (GTK_ENTRY (entry)) &&
		strlen (gtk_entry_get_text (GTK_ENTRY (entry))) > 0)
	{
		*return_value = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	}
	else
	{
		*return_value = NULL;
	}
	gtk_widget_destroy (dialog);	
	return (res == GTK_RESPONSE_OK);
}

gboolean
anjuta_util_prog_is_installed (gchar * prog, gboolean show)
{
	gchar* prog_path = g_find_program_in_path (prog);
	if (prog_path)
	{
		g_free (prog_path);
		return TRUE;
	}
	if (show)
	{
		anjuta_util_dialog_error (NULL, _("The \"%s\" utility is not installed.\n"
					  "Please install it."), prog);
	}
	return FALSE;
}

gchar *
anjuta_util_get_a_tmp_file (void)
{
	static gint count = 0;
	gchar *filename;
	const gchar *tmpdir;

	tmpdir = g_get_tmp_dir ();
	filename =
		g_strdup_printf ("%s/anjuta_%d.%d", tmpdir, count++, getpid ());
	return filename;
}

/* GList of strings operations */
GList *
anjuta_util_glist_from_string (const gchar *string)
{
	gchar *str, *temp, buff[256];
	GList *list;
	gchar *word_start, *word_end;
	gboolean the_end;

	list = NULL;
	the_end = FALSE;
	temp = g_strdup (string);
	str = temp;
	if (!str)
		return NULL;

	while (1)
	{
		gint i;
		gchar *ptr;

		/* Remove leading spaces */
		while (isspace (*str) && *str != '\0')
			str++;
		if (*str == '\0')
			break;

		/* Find start and end of word */
		word_start = str;
		while (!isspace (*str) && *str != '\0')
			str++;
		word_end = str;

		/* Copy the word into the buffer */
		for (ptr = word_start, i = 0; ptr < word_end; ptr++, i++)
			buff[i] = *ptr;
		buff[i] = '\0';
		if (strlen (buff))
			list = g_list_append (list, g_strdup (buff));
		if (*str == '\0')
			break;
	}
	if (temp)
		g_free (temp);
	return list;
}

/* Prefix the strings */
void
anjuta_util_glist_strings_prefix (GList * list, const gchar *prefix)
{
	GList *node;
	node = list;
	
	g_return_if_fail (prefix != NULL);
	while (node)
	{
		gchar* tmp;
		tmp = node->data;
		node->data = g_strconcat (prefix, tmp, NULL);
		if (tmp) g_free (tmp);
		node = g_list_next (node);
	}
}

/* Suffix the strings */
void
anjuta_util_glist_strings_sufix (GList * list, const gchar *sufix)
{
	GList *node;
	node = list;
	
	g_return_if_fail (sufix != NULL);
	while (node)
	{
		gchar* tmp;
		tmp = node->data;
		node->data = g_strconcat (tmp, sufix, NULL);
		if (tmp) g_free (tmp);
		node = g_list_next (node);
	}
}

/* Duplicate list of strings */
GList*
anjuta_util_glist_strings_dup (GList * list)
{
	GList *node;
	GList *new_list;
	
	new_list = NULL;
	node = list;
	while (node)
	{
		if (node->data)
			new_list = g_list_append (new_list, g_strdup(node->data));
		else
			new_list = g_list_append (new_list, NULL);
		node = g_list_next (node);
	}
	return new_list;
}

static gchar*
get_real_path(const gchar *file_name)
{
	if (file_name)
	{
		gchar path[PATH_MAX+1];
		memset(path, '\0', PATH_MAX+1);
		realpath(file_name, path);
		return g_strdup(path);
	}
	else
		return NULL;
}

/* Dedup a list of paths - duplicates are removed from the tail.
** Useful for deduping Recent Files and Recent Projects */
GList*
anjuta_util_glist_path_dedup(GList *list)
{
	GList *nlist = NULL, *tmp, *tmp1;
	gchar *path;
	struct stat s;
	for (tmp = list; tmp; tmp = g_list_next(tmp))
	{
		path = get_real_path((const gchar *) tmp->data);
		if (0 != stat(path, &s))
		{
			g_free(path);
		}
		else
		{
			for (tmp1 = nlist; tmp1; tmp1 = g_list_next(tmp1))
			{
				if (0 == strcmp((const char *) tmp1->data, path))
				{
					g_free(path);
					path = NULL;
					break;
				}
			}
			if (path)
			nlist = g_list_prepend(nlist, path);
		}
	}
	anjuta_util_glist_strings_free(list);
	nlist = g_list_reverse(nlist);
	return nlist;
}

static gint
sort_node (gchar* a, gchar *b)
{
	if ( !a && !b) return 0;
	else if (!a) return -1;
	else if (!b) return 1;
	return strcmp (a, b);
}

/* Sort the list alphabatically */
GList*
anjuta_util_glist_strings_sort (GList * list)
{
	return g_list_sort(list, (GCompareFunc)sort_node);
}

/* Free the strings and GList */
void
anjuta_util_glist_strings_free (GList * list)
{
	GList *node;
	node = list;
	while (node)
	{
		if (node->data)
			g_free (node->data);
		node = g_list_next (node);
	}
	g_list_free (list);
}

int
anjuta_util_type_from_string (AnjutaUtilStringMap *map, const char *str)
{
	int i = 0;

	while (-1 != map[i].type)
	{
		if (0 == strcmp(map[i].name, str))
			return map[i].type;
		++ i;
	}
	return -1;
}

const char*
anjuta_util_string_from_type (AnjutaUtilStringMap *map, int type)
{
	int i = 0;
	while (-1 != map[i].type)
	{
		if (map[i].type == type)
			return map[i].name;
		++ i;
	}
	return "";
}

GList*
anjuta_util_glist_from_map (AnjutaUtilStringMap *map)
{
	GList *out_list = NULL;
	int i = 0;
	while (-1 != map[i].type)
	{
		out_list = g_list_append(out_list, map[i].name);
		++ i;
	}
	return out_list;
}


GList *
anjuta_util_update_string_list (GList *p_list, const gchar *p_str, gint length)
{
	gint i;
	gchar *str;
	if (!p_str)
		return p_list;
	for (i = 0; i < g_list_length (p_list); i++)
	{
		str = (gchar *) g_list_nth_data (p_list, i);
		if (!str)
			continue;
		if (strcmp (p_str, str) == 0)
		{
			p_list = g_list_remove (p_list, str);
			p_list = g_list_prepend (p_list, str);
			return p_list;
		}
	}
	p_list = g_list_prepend (p_list, g_strdup (p_str));
	while (g_list_length (p_list) > length)
	{
		str = g_list_nth_data (p_list, g_list_length (p_list) - 1);
		p_list = g_list_remove (p_list, str);
		g_free (str);
	}
	return p_list;
}

gboolean
anjuta_util_create_dir (const gchar * d)
{
	if (g_file_test (d, G_FILE_TEST_IS_DIR))
		return TRUE;
	if (mkdir (d, 0755))
		return FALSE;
	return TRUE;
}

pid_t
anjuta_util_execute_shell (const gchar *dir, const gchar *command)
{
	pid_t pid;
	gchar *shell;
	
	g_return_val_if_fail (command != NULL, -1);
	
	shell = gnome_util_user_shell ();
	pid = fork();
	if (pid == 0)
	{
		if(dir)
		{
			anjuta_util_create_dir (dir);
			chdir (dir);
		}
		execlp (shell, shell, "-c", command, NULL);
		g_warning (_("Cannot execute command: %s (using shell %s)\n"), command, shell);
		_exit(1);
	}
	if (pid < 0)
		g_warning (_("Cannot execute command %s (using shell %s)\n"), command, shell);
	g_free (shell);
	// Anjuta will take care of child exit automatically.
	return pid;
}

gchar *
anjuta_util_convert_to_utf8 (const gchar *str)
{
	GError *error = NULL;
	gchar *utf8_msg_string = NULL;
	
	g_return_val_if_fail (str != NULL, NULL);
	g_return_val_if_fail (strlen (str) > 0, NULL);
	
	if (g_utf8_validate(str, -1, NULL))
	{
		utf8_msg_string = g_strdup (str);
	}
	else
	{
		gsize rbytes, wbytes;
		utf8_msg_string = g_locale_to_utf8 (str, -1, &rbytes, &wbytes, &error);
		if (error != NULL) {
			g_warning ("g_locale_to_utf8 failed: %s\n", error->message);
			g_error_free (error);
			g_free (utf8_msg_string);
			return NULL;
		}
	}
	return utf8_msg_string;
}

GList*
anjuta_util_parse_args_from_string (const gchar* string)
{
	gboolean escaped;
	gchar    quote;
	gchar    buffer[2048];
	const gchar *s;
	gint     idx;
	GList* args = NULL;
	
	idx = 0;
	escaped = FALSE;
	quote = (gchar)-1;
	s = string;
	
	while (*s) {
		if (!isspace(*s))
			break;
		s++;
	}

	while (*s) {
		if (escaped) {
			/* The current char was escaped */
			buffer[idx++] = *s;
			escaped = FALSE;
		} else if (*s == '\\') {
			/* Current char is an escape */
			escaped = TRUE;
		} else if (*s == quote) {
			/* Current char ends a quotation */
			quote = (gchar)-1;
			if (!isspace(*(s+1)) && (*(s+1) != '\0')) {
				/* If there is no space after the quotation or it is not
				   the end of the string */
				g_warning ("Parse error while parsing program arguments");
			}
		} else if ((*s == '\"' || *s == '\'')) {
			if (quote == (gchar)-1) {
				/* Current char starts a quotation */
				quote = *s;
			} else {
				/* Just a quote char inside quote */
				buffer[idx++] = *s;
			}
		} else if (quote > 0){
			/* Any other char inside quote */
			buffer[idx++] = *s;
		} else if (isspace(*s)) {
			/* Any white space outside quote */
			if (idx > 0) {
				buffer[idx++] = '\0';
				args = g_list_append (args, g_strdup (buffer));
				idx = 0;
			}
		} else {
			buffer[idx++] = *s;
		}
		s++;
	}
	if (idx > 0) {
		/* There are chars in the buffer. Flush as the last arg */
		buffer[idx++] = '\0';
		args = g_list_append (args, g_strdup (buffer));
		idx = 0;
	}
	if (quote > 0) {
		g_warning ("Unclosed quotation encountered at the end of parsing");
	}
	return args;
}

gchar*
anjuta_util_escape_quotes(const gchar* str)
{
	gchar *buffer;
	gint idx, max_size;
	const gchar *s = str;
	
	g_return_val_if_fail(str, NULL);
	idx = 0;
	
	/* We are assuming there will be less than 2048 chars to escape */
	max_size = strlen(str) + 2048;
	buffer = g_new (gchar, max_size);
	max_size -= 2;
	
	while(*s) {
		if (idx > max_size)
			break;
		if (*s == '\"' || *s == '\'' || *s == '\\')
			buffer[idx++] = '\\';
		buffer[idx++] = *s;
		s++;
	}
	buffer[idx] = '\0';
	return buffer;
}

/* Diff the text contained in uri with text. Return true if files
differ, FALSE if they are identical.
FIXME: Find a better algorithm, this seems ineffective */

gboolean anjuta_util_diff(const gchar* uri, const gchar* text)
{
	GnomeVFSFileSize bytes_read;
	
	gchar* file_text;
	
	GnomeVFSHandle* handle = NULL;
	GnomeVFSFileInfo info;
	
	gnome_vfs_get_file_info(uri, &info, GNOME_VFS_FILE_INFO_DEFAULT);
	
	file_text = g_new0(gchar, info.size + 1);
	
	if (gnome_vfs_open(&handle, uri, GNOME_VFS_OPEN_READ != GNOME_VFS_OK))
		return TRUE;
	
	if ((gnome_vfs_read(handle, file_text, info.size, &bytes_read) == GNOME_VFS_OK)
		&& (bytes_read == info.size))
	{
		gnome_vfs_close(handle);
		DEBUG_PRINT("File text: %d", strlen(file_text));
		DEBUG_PRINT("text: %d", strlen(text));
		
		if ((strlen(file_text) == strlen(text))
			&& strcmp(file_text, text) == 0)
			return FALSE;
		return TRUE;
	}
	else
	{
		gnome_vfs_close(handle);
		return TRUE;
	}
}

/*
 * Sun specific implementations
 *
 */
#ifdef sun
#include <grp.h>

static int ptym_open (char *pts_name);
static int ptys_open (int fdm, char * pts_name);

int
login_tty(int ttyfd)
{
  int fd;
  char *fdname;

#ifdef HAVE_SETSID
  setsid();
#endif
#ifdef HAVE_SETPGID
  setpgid(0, 0);
#endif
	
  /* First disconnect from the old controlling tty. */
#ifdef TIOCNOTTY
  fd = open("/dev/tty", O_RDWR|O_NOCTTY);
  if (fd >= 0)
    {
      ioctl(fd, TIOCNOTTY, NULL);
      close(fd);
    }
  else
    //syslog(LOG_WARNING, "NO CTTY");
#endif /* TIOCNOTTY */
  
  /* Verify that we are successfully disconnected from the controlling tty. */
  fd = open("/dev/tty", O_RDWR|O_NOCTTY);
  if (fd >= 0)
    {
      //syslog(LOG_WARNING, "Failed to disconnect from controlling tty.");
      close(fd);
    }
  
  /* Make it our controlling tty. */
#ifdef TIOCSCTTY
  ioctl(ttyfd, TIOCSCTTY, NULL);
#endif /* TIOCSCTTY */

  fdname = ttyname (ttyfd);
  fd = open(fdname, O_RDWR);
  if (fd < 0)
    ;//syslog(LOG_WARNING, "open %s: %s", fdname, strerror(errno));
  else
    close(fd);

  /* Verify that we now have a controlling tty. */
  fd = open("/dev/tty", O_WRONLY);
  if (fd < 0)
    {
      //syslog(LOG_WARNING, "open /dev/tty: %s", strerror(errno));
      return 1;
    }

  close(fd);
#if defined(HAVE_VHANGUP) && !defined(HAVE_REVOKE)
  {
    RETSIGTYPE (*sig)();
    sig = signal(SIGHUP, SIG_IGN);
    vhangup();
    signal(SIGHUP, sig);
  }
#endif
  fd = open(fdname, O_RDWR);
  if (fd == -1)
    {
      //syslog(LOG_ERR, "can't reopen ctty %s: %s", fdname, strerror(errno));
      return -1;
    }
	
  close(ttyfd);

  if (fd != 0)
    close(0);
  if (fd != 1)
    close(1);
  if (fd != 2)
    close(2);

  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
  if (fd > 2)
    close(fd);
  return 0;
}

int
openpty(int *amaster, int *aslave, char *name, struct termios *termp,
		struct winsize *winp)
{
	char line[20];
	*amaster = ptym_open(line);
	if (*amaster < 0)
		return -1;
	*aslave = ptys_open(*amaster, line);
	if (*aslave < 0) {
		close(*amaster);
		return -1;
	}
	if (name)
		strcpy(name, line);
#ifndef TCSAFLUSH
#define TCSAFLUSH TCSETAF
#endif
	if (termp)
		(void) tcsetattr(*aslave, TCSAFLUSH, termp);
#ifdef TIOCSWINSZ
	if (winp)
		(void) ioctl(*aslave, TIOCSWINSZ, (char *)winp);
#endif
	return 0;
}

static int
ptym_open(char * pts_name)
{
	int fdm;
#ifdef HAVE_PTSNAME
	char *ptr;

	strcpy(pts_name, "/dev/ptmx");
	fdm = open(pts_name, O_RDWR);
	if (fdm < 0)
		return -1;
	if (grantpt(fdm) < 0) { /* grant access to slave */
		close(fdm);
		return -2;
	}
	if (unlockpt(fdm) < 0) { /* clear slave's lock flag */
		close(fdm);
		return -3;
	}
	ptr = ptsname(fdm);
	if (ptr == NULL) { /* get slave's name */
		close (fdm);
		return -4;
	}
	strcpy(pts_name, ptr); /* return name of slave */
	return fdm;            /* return fd of master */
#else
	char *ptr1, *ptr2;

	strcpy(pts_name, "/dev/ptyXY");
	/* array index: 012345689 (for references in following code) */
	for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1 != 0; ptr1++) {
		pts_name[8] = *ptr1;
		for (ptr2 = "0123456789abcdef"; *ptr2 != 0; ptr2++) {
			pts_name[9] = *ptr2;
			/* try to open master */
			fdm = open(pts_name, O_RDWR);
			if (fdm < 0) {
				if (errno == ENOENT) /* different from EIO */
					return -1;  /* out of pty devices */
				else
					continue;  /* try next pty device */
			}
			pts_name[5] = 't'; /* chage "pty" to "tty" */
			return fdm;   /* got it, return fd of master */
		}
	}
	return -1; /* out of pty devices */
#endif
}

static int
ptys_open(int fdm, char * pts_name)
{
	int fds;
#ifdef HAVE_PTSNAME
	/* following should allocate controlling terminal */
	fds = open(pts_name, O_RDWR);
	if (fds < 0) {
		close(fdm);
		return -5;
	}
	if (ioctl(fds, I_PUSH, "ptem") < 0) {
		close(fdm);
		close(fds);
		return -6;
	}
	if (ioctl(fds, I_PUSH, "ldterm") < 0) {
		close(fdm);
		close(fds);
		return -7;
	}
	if (ioctl(fds, I_PUSH, "ttcompat") < 0) {
		close(fdm);
		close(fds);
		return -8;
	}

	if (ioctl(fdm, I_PUSH, "pckt") < 0) {
		close(fdm);
		close(fds);
		return -8;
	}
	
	if (ioctl(fdm, I_SRDOPT, RMSGN|RPROTDAT) < 0) {
		close(fdm);
		close(fds);
		return -8;
	}

	return fds;
#else
	int gid;
	struct group *grptr;

	grptr = getgrnam("tty");
	if (grptr != NULL)
		gid = grptr->gr_gid;
	else
		gid = -1;  /* group tty is not in the group file */
	/* following two functions don't work unless we're root */
	chown(pts_name, getuid(), gid);
	chmod(pts_name, S_IRUSR | S_IWUSR | S_IWGRP);
	fds = open(pts_name, O_RDWR);
	if (fds < 0) {
		close(fdm);
		return -1;
	}
	return fds;
#endif
}

int
forkpty(int *amaster, char *name, struct termios *termp, struct winsize *winp)
{
  int master, slave, pid;

  if (openpty(&master, &slave, name, termp, winp) == -1)
    return (-1);
  switch (pid = fork()) {
  case -1:
    return (-1);
  case 0:
    /*
     * child
     */
    close(master);
    login_tty(slave);
    return (0);
  }
  /*
   * parent
   */
  *amaster = master;
  close(slave);
  return (pid);
}

int scandir(const char *dir, struct dirent ***namelist,
            int (*select)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **))
{
  DIR *d;
  struct dirent *entry;
  register int i=0;
  size_t entrysize;

  if ((d=opendir(dir)) == NULL)
     return(-1);

  *namelist=NULL;
  while ((entry=readdir(d)) != NULL)
  {
    if (select == NULL || (select != NULL && (*select)(entry)))
    {
      *namelist=(struct dirent **)realloc((void *)(*namelist),
                 (size_t)((i+1)*sizeof(struct dirent *)));
	if (*namelist == NULL) return(-1);
	entrysize=sizeof(struct dirent)-sizeof(entry->d_name)+strlen(entry->d_name)+1;
	(*namelist)[i]=(struct dirent *)malloc(entrysize);
	if ((*namelist)[i] == NULL) return(-1);
	memcpy((*namelist)[i], entry, entrysize);
	i++;
    }
  }
  if (closedir(d)) return(-1);
  if (i == 0) return(-1);
  if (compar != NULL)
    qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), compar);
    
  return(i);
}

int alphasort (const struct dirent **a, const struct dirent **b)
{
    return (strcmp((*a)->d_name, (*b)->d_name));
}
#endif /* sun */
