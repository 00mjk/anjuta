/*
    utilities.c
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib.h>

#include <gtk/gtk.h>
#include "anjuta.h"
#include "utilities.h"
#include "launcher.h"
#include "properties.h"
#include "resources.h"

gchar *
extract_filename (gchar * full_filename)
{

	gint i;
	gint length;

	if (!full_filename)
		return NULL;
	length = strlen (full_filename);
	if (length == 0)
		return full_filename;
	if (full_filename[length - 1] == '/')
		return &full_filename[length];
	for (i = strlen (full_filename) - 1; i >= 0; i--)
		if (full_filename[i] == '/')
			break;
	if (i == 0 && full_filename[0] != '/')
		return full_filename;
	return &full_filename[++i];
}

#define NSTR(S) ((S)?(S):"NULL")
gchar *
resolved_file_name(gchar *full_filename)
{
	char real_filename[PATH_MAX], *final_filename;
	if (!full_filename)
		final_filename = NULL;
	else if (realpath(full_filename, real_filename))
		final_filename = g_strdup(real_filename);
	else
		final_filename = g_strdup(full_filename);
	g_message("%s -> %s", NSTR(full_filename), NSTR(final_filename));
	return final_filename;
}

GList *
update_string_list (GList * list, gchar * string, gint length)
{
	gint i;
	gchar *str;
	if (!string)
		return list;
	for (i = 0; i < g_list_length (list); i++)
	{
		str = (gchar *) g_list_nth_data (list, i);
		if (!str)
			continue;
		if (strcmp (string, str) == 0)
		{
			list = g_list_remove (list, str);
			list = g_list_prepend (list, str);
			return list;
		}
	}
	list = g_list_prepend (list, g_strdup (string));
	while (g_list_length (list) > length)
	{
		str = g_list_nth_data (list, g_list_length (list) - 1);
		list = g_list_remove (list, str);
		g_free (str);
	}
	return list;
}

void
free_string_list ( GList * pList )
{
	int i ;

	if( pList )
	{
		for (i = 0; i < g_list_length (pList); i++)
			g_free (g_list_nth (pList, i)->data);
		
		g_list_free (pList);
	}
}



gboolean
parse_error_line (const gchar * line, gchar ** filename, int *lineno)
{
	gint i = 0;
	gint j = 0;
	gint k = 0;
	gchar *dummy;

	while (line[i++] != ':')
	{
		if (i >= strlen (line) || i >= 512 || line[i - 1] == ' ')
		{
			goto down;
		}
	}
	if (isdigit (line[i]))
	{
		j = i;
		while (isdigit (line[i++])) ;
		dummy = g_strndup (&line[j], i - j - 1);
		*lineno = atoi (dummy);
		if (dummy)
			g_free (dummy);
		dummy = g_strndup (line, j - 1);
		*filename = g_strdup (g_strstrip (dummy));
		if (dummy)
			g_free (dummy);
		return TRUE;
	}

      down:
	i = strlen (line) - 1;
	while (isspace (line[i]) == FALSE)
	{
		i--;
		if (i < 0)
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	}
	k = i++;
	while (line[i++] != ':')
	{
		if (i >= strlen (line) || i >= 512 || line[i - 1] == ' ')
		{
			*filename = NULL;
			*lineno = 0;
			return FALSE;
		}
	}
	if (isdigit (line[i]))
	{
		j = i;
		while (isdigit (line[i++])) ;
		dummy = g_strndup (&line[j], i - j - 1);
		*lineno = atoi (dummy);
		if (dummy)
			g_free (dummy);
		dummy = g_strndup (&line[k], j - k - 1);
		*filename = g_strdup (g_strstrip (dummy));
		if (dummy)
			g_free (dummy);
		return TRUE;
	}
	*lineno = 0;
	*filename = NULL;
	return FALSE;
}

gchar *
get_file_extension (gchar * file)
{
	gchar *pos;
	if (!file)
		return NULL;
	pos = strrchr (file, '.');
	if (pos)
		return ++pos;
	else
		return NULL;
}

FileExtType get_file_ext_type (gchar * file)
{
	gchar *pos, *filetype_str;
	FileExtType filetype;

	if (!file)
		return FILE_TYPE_UNKNOWN;
	pos = get_file_extension (file);
	if (pos == NULL)
		return FILE_TYPE_UNKNOWN;

	filetype_str =
		prop_get_new_expand (app->preferences->props, "filetype.",
				     file);
	if (filetype_str == NULL)
		return FILE_TYPE_UNKNOWN;

	if (strcmp (filetype_str, "c") == 0)
	{
		filetype = FILE_TYPE_C;
	}
	else if (strcmp (filetype_str, "cpp") == 0)
	{
		filetype = FILE_TYPE_CPP;
	}
	else if (strcmp (filetype_str, "header") == 0)
	{
		filetype = FILE_TYPE_HEADER;
	}
	else if (strcmp (filetype_str, "pascal") == 0)
	{
		filetype = FILE_TYPE_PASCAL;
	}
	else if (strcmp (filetype_str, "rc") == 0)
	{
		filetype = FILE_TYPE_RC;
	}
	else if (strcmp (filetype_str, "idl") == 0)
	{
		filetype = FILE_TYPE_IDL;
	}
	else if (strcmp (filetype_str, "cs") == 0)
	{
		filetype = FILE_TYPE_CS;
	}
	else if (strcmp (filetype_str, "java") == 0)
	{
		filetype = FILE_TYPE_JAVA;
	}
	else if (strcmp (filetype_str, "js") == 0)
	{
		filetype = FILE_TYPE_JS;
	}
	else if (strcmp (filetype_str, "conf") == 0)
	{
		filetype = FILE_TYPE_CONF;
	}
	else if (strcmp (filetype_str, "hypertext") == 0)
	{
		filetype = FILE_TYPE_HTML;
	}
	else if (strcmp (filetype_str, "xml") == 0)
	{
		filetype = FILE_TYPE_XML;
	}
	else if (strcmp (filetype_str, "latex") == 0)
	{
		filetype = FILE_TYPE_LATEX;
	}
	else if (strcmp (filetype_str, "lua") == 0)
	{
		filetype = FILE_TYPE_LUA;
	}
	else if (strcmp (filetype_str, "perl") == 0)
	{
		filetype = FILE_TYPE_PERL;
	}
	else if (strcmp (filetype_str, "shellscript") == 0)
	{
		filetype = FILE_TYPE_SH;
	}
	else if (strcmp (filetype_str, "python") == 0)
	{
		filetype = FILE_TYPE_PYTHON;
	}
	else if (strcmp (filetype_str, "ruby") == 0)
	{
		filetype = FILE_TYPE_RUBY;
	}
	else if (strcmp (filetype_str, "props") == 0)
	{
		filetype = FILE_TYPE_PROPERTIES;
	}
	else if (strcmp (filetype_str, "project") == 0)
	{
		filetype = FILE_TYPE_PROJECT;
	}
	else if (strcmp (filetype_str, "batch") == 0)
	{
		filetype = FILE_TYPE_BATCH;
	}
	else if (strcmp (filetype_str, "errorlist") == 0)
	{
		filetype = FILE_TYPE_ERRORLIST;
	}
	else if (strcmp (filetype_str, "makefile") == 0)
	{
		filetype = FILE_TYPE_MAKEFILE;
	}
	else if (strcmp (filetype_str, "iface") == 0)
	{
		filetype = FILE_TYPE_IFACE;
	}
	else if (strcmp (filetype_str, "diff") == 0)
	{
		filetype = FILE_TYPE_DIFF;
	}
	else if (strcmp (filetype_str, "icon") == 0)
	{
		filetype = FILE_TYPE_ICON;
	}
	else if (strcmp (filetype_str, "image") == 0)
	{
		filetype = FILE_TYPE_IMAGE;
	}
	else if (strcmp (filetype_str, "asm") == 0)
	{
		filetype = FILE_TYPE_ASM;
	}
	else if (strcmp (filetype_str, "scm") == 0)
	{
		filetype = FILE_TYPE_SCM;
	}
	else if (strcmp (filetype_str, "po") == 0)
	{
		filetype = FILE_TYPE_PO;
	}
	else if (strcmp (filetype_str, "sql") == 0)
	{
		filetype = FILE_TYPE_SQL;
	}
	else if (strcmp (filetype_str, "plsql") == 0)
	{
		filetype = FILE_TYPE_PLSQL;
	}
	else if (strcmp (filetype_str, "vb") == 0)
	{
		filetype = FILE_TYPE_VB;
	}
	else if (strcmp (filetype_str, "baan") == 0)
	{
		filetype = FILE_TYPE_BAAN;
	}
	else if (strcmp (filetype_str, "ada") == 0)
	{
		filetype = FILE_TYPE_ADA;
	}
	else if (strcmp (filetype_str, "wscript") == 0)
	{
		filetype = FILE_TYPE_WSCRIPT;
	}
	else
	{
		filetype = FILE_TYPE_UNKNOWN;
	}
	g_free (filetype_str);
	return filetype;
}

gchar *
get_a_tmp_file ()
{
	static gint count;
	gchar *filename;
	filename =
		g_strdup_printf ("%s/anjuta_%d.%d", app->dirs->tmp, count++,
				 getpid ());
	return filename;
}

gboolean write_line (FILE * stream, gchar * str)
{
	unsigned long len;

	if (!str || !stream)
		return FALSE;
	len = strlen (str);
	if (len != 0)
		if (fwrite (str, len, sizeof (gchar), stream) < 0)
			return FALSE;
	if (fwrite ("\n", 1, sizeof (gchar), stream) < 1)
		return FALSE;
	return TRUE;
}

gboolean read_line (FILE * stream, gchar ** str)
{
	unsigned long count;
	gchar buffer[1024];
	gint ch;

	if (stream == NULL)
		return FALSE;

	count = 0;
	while (1)
	{
		ch = fgetc (stream);
		if (ch < 0)
			break;
		if (ch == '\n')
			break;
		buffer[count] = (char) ch;
		count++;
		if (count >= 1024 - 1)
			break;
	}
	buffer[count] = '\0';
	*str = g_strdup (buffer);
	return TRUE;
}

gboolean write_string (FILE * stream, gchar * t, gchar * str)
{
	unsigned long len;
	if (!str || !stream)
		return FALSE;
	len = strlen (str);
	if (fprintf (stream, "%s :%lu: ", t, len) < 2)
		return FALSE;
	if (len != 0)
		if (fwrite (str, len, sizeof (gchar), stream) < 0)
			return FALSE;
	if (fwrite ("\n", 1, sizeof (gchar), stream) < 1)
		return FALSE;
	return TRUE;
}

gboolean read_string (FILE * stream, gchar * t, gchar ** str)
{
	unsigned long tmp;
	gchar *buff, bb[3], token[256];

	tmp = 0;
	strcpy (token, "");
	if (stream == NULL)
		return FALSE;
	if (fscanf (stream, "%s :%lu: ", token, &tmp) < 2)
		return FALSE;
	if (strcmp (token, t) != 0)
		return FALSE;
	if (tmp == 0)
	{
		if (str)
			(*str) = g_strdup ("");
		return TRUE;
	}
	buff = g_malloc ((tmp + 1) * sizeof (char));
	if (fread (buff, tmp, sizeof (gchar), stream) < 0)
	{
		g_free (buff);
		return FALSE;
	}
	if (fread (bb, 1, sizeof (gchar), stream) < 1)
	{
		g_free (buff);
		return FALSE;
	}
	buff[tmp] = '\0';
	if (str)
		(*str) = buff;
	else
		g_free (buff);
	return TRUE;
}

gint compare_string_func (gconstpointer a, gconstpointer b)
{
	if (!a && !b)
		return 0;
	if (!a || !b)
		return -1;
	return (strcmp ((char *) a, (char *) b));
}

gboolean
file_is_regular (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISREG (st.st_mode))
		return TRUE;
	return FALSE;
}

gboolean
file_is_directory (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISDIR (st.st_mode))
		return TRUE;
	return FALSE;
}

gboolean
file_is_link (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = lstat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISLNK (st.st_mode))
		return TRUE;
	return FALSE;
}

gboolean
file_is_char_device (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISCHR (st.st_mode))
		return TRUE;
	return FALSE;
}

gboolean
file_is_block_device (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISBLK (st.st_mode))
		return TRUE;
	return FALSE;
}

gboolean
file_is_fifo (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISFIFO (st.st_mode))
		return TRUE;
	return FALSE;
}

gboolean
file_is_socket (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISSOCK (st.st_mode))
		return TRUE;
	return FALSE;
}

gboolean
file_is_readable (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_IRUSR & st.st_mode)
		return TRUE;
	return FALSE;
}

gboolean
file_is_readonly (const gchar * fn)
{
	return file_is_readable (fn) && !file_is_writable (fn);
}

gboolean
file_is_readwrite (const gchar * fn)
{
	return file_is_readable (fn) && file_is_writable (fn);
}


gboolean
file_is_writable (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_IWUSR & st.st_mode)
		return TRUE;
	return FALSE;
}

gboolean
file_is_executable (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_IXUSR & st.st_mode)
		return TRUE;
	return FALSE;
}

gboolean
file_is_suid (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISUID & st.st_mode)
		return TRUE;
	return FALSE;
}

gboolean
file_is_sgid (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISGID & st.st_mode)
		return TRUE;
	return FALSE;
}

gboolean
file_is_sticky (const gchar * fn)
{
	struct stat st;
	int ret;
	if (!fn)
		return FALSE;
	ret = stat (fn, &st);
	if (ret)
		return FALSE;
	if (S_ISVTX & st.st_mode)
		return TRUE;
	return FALSE;
}

gboolean
copy_file (gchar * src, gchar * dest, gboolean show_error)
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
			anjuta_system_error (errno, _("Couldn't read file: %s."), src);
		return FALSE;
	}
	
	output_fp = fopen (dest, "wb");
	if (output_fp == NULL)
	{
		if( show_error)
			anjuta_system_error (errno, _("Couldn't create file: %s."), dest);
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
		anjuta_system_error (errno, _("Couldn't complete file copy"));
	return error;
}

void
update_gtk ()
{
	/* Do not update gtk when launcher is busy */
	/* This will freeze the application till the launcher is done */
	if (launcher_is_busy () == TRUE)
		return;
	if (app->auto_gtk_update == FALSE)
		return;
	while (gtk_events_pending ())
	{
		gtk_main_iteration ();
	}
}

void
entry_set_text_n_select (GtkWidget * entry, gchar * text,
			 gboolean use_selection)
{
	gchar *chars;

	if (!entry)
		return;
	if (GTK_IS_ENTRY (entry) == FALSE)
		return;

	chars = anjuta_get_current_selection ();
	if (use_selection)
	{
		if (chars)
		{
			gtk_entry_set_text (GTK_ENTRY (entry), chars);
			gtk_editable_select_region (GTK_EDITABLE (entry), 0,
						    strlen (gtk_entry_get_text
							    (GTK_ENTRY
							     (entry))));
		}
		else
		{
			if (text)
				gtk_entry_set_text (GTK_ENTRY (entry), text);
			gtk_editable_select_region (GTK_EDITABLE (entry), 0,
						    strlen (gtk_entry_get_text
							    (GTK_ENTRY
							     (entry))));
		}
	}
	else
	{
		if (text)
			gtk_entry_set_text (GTK_ENTRY (entry), text);
		gtk_editable_select_region (GTK_EDITABLE (entry), 0,
					    strlen (gtk_entry_get_text
						    (GTK_ENTRY (entry))));
	}
	if (chars)
		g_free (chars);
}

gboolean
force_create_dir (gchar * d)
{
	if (file_is_directory (d))
		return TRUE;
	if (mkdir (d, 0755))
		return FALSE;
	return TRUE;
}

GdkFont *
get_fixed_font ()
{
	static GdkFont *font;
	static gint done;

	if (done)
		return font;
	font =
		gdk_font_load
		("-misc-fixed-medium-r-semicondensed-*-*-120-*-*-c-*-iso8859-9");
	done = 1;
	if (font)
	{
		gdk_font_ref (font);
		return font;
	}
	g_warning ("Cannot load fixed(misc) font. Using default font.");
	return NULL;
}

gchar *
remove_white_spaces (gchar * text)
{
	guint src_count, dest_count, tab_count;
	gchar buff[2048];	/* Let us hope that it does not overflow */

	tab_count = preferences_get_int (app->preferences, TAB_SIZE);
	dest_count = 0;
	for (src_count = 0; src_count < strlen (text); src_count++)
	{
		if (text[src_count] == '\t')
		{
			gint j;
			for (j = 0; j < tab_count; j++)
				buff[dest_count++] = ' ';
		}
		else if (isspace (text[src_count]))
		{
			buff[dest_count++] = ' ';
		}
		else
		{
			buff[dest_count++] = text[src_count];
		}
	}
	buff[dest_count] = '\0';
	return g_strdup (buff);
}

GList *
remove_blank_lines (GList * lines)
{
	GList *list, *node;
	gchar *str;

	if (lines)
		list = g_list_copy (lines);
	else
		list = NULL;

	node = list;
	while (node)
	{
		str = node->data;
		node = g_list_next (node);
		if (!str)
		{
			list = g_list_remove (list, str);
			continue;
		}
		if (strlen (g_strchomp (str)) < 1)
			list = g_list_remove (list, str);
	}
	return list;
}

gboolean widget_is_child (GtkWidget * parent, GtkWidget * child)
{
	if (GTK_IS_CONTAINER (parent))
	{
		GList *children;
		GList *node;
		children = gtk_container_children (GTK_CONTAINER (parent));
		node = children;
		while (node)
		{
			if (widget_is_child (node->data, child))
				return TRUE;
		}
	}
	else
	{
		if (parent == child)
			return TRUE;
	}
	return FALSE;
}

GList *
glist_from_string (gchar * string)
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

GList *
glist_from_data (guint props, gchar * id)
{
	gchar *str;
	GList *list;

	str = prop_get (props, id);
	list = glist_from_string (str);
	return list;
}

void
glist_strings_free (GList * list)
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

GList*
glist_strings_dup (GList * list)
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

void
string_free(gchar* str)
{
	if (str) g_free(str);
}

void
string_assign (gchar ** string, gchar * value)
{
	if (*string)
		g_free (*string);
	*string = NULL;
	if (value)
		*string = g_strdup (value);
}
void
glist_strings_prefix (GList * list, gchar *prefix)
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

void
glist_strings_sufix (GList * list, gchar *sufix)
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


static gint
sort_node (gchar* a, gchar *b)
{
	if ( !a && !b) return 0;
	else if (!a) return -1;
	else if (!b) return 1;
	return strcmp (a, b);
}

GList*
glist_strings_sort (GList * list)
{
	return g_list_sort(list, (GCompareFunc)sort_node);
}

gchar*
get_swapped_filename(gchar* filename)
{
	size_t len;
	gchar *newfname;

	g_return_val_if_fail (filename != NULL, NULL);

	len=strlen(filename);
	newfname=g_malloc(len+5); /* to be on the safer side */
	while(len)
	{
		if(filename[len]=='.')
			break; 
		len--;
	}
	len++;
	strcpy(newfname, filename);
	if(strncasecmp(&filename[len],"h", 1)==0)
	{
		strcpy(&newfname[len],"cc");
		if(file_is_regular(newfname)) /* test if *.cc exits */
		{
			return newfname;
		}
		strcpy(&newfname[len],"cpp");
		if(file_is_regular(newfname)) /* test if *.cpp exits */
		{
			return newfname;
		}
		strcpy(&newfname[len],"cxx");
		if(file_is_regular(newfname))     /* test if *.cxx exist */
		{
			return newfname;
		}
		strcpy(&newfname[len],"c");
		if(file_is_regular(newfname))     /* test if *.c exist */
		{ 
			return newfname;
		}
	}
	else if(strncasecmp(&filename[len],"c",1)==0 )
	{
		strcpy(&newfname[len],"h");
		if(file_is_regular(newfname))      /* test if *.h exist */
		{ 
			return newfname;
		}
		strcpy(&newfname[len],"hh");
		if(file_is_regular(newfname))      /* test if *.hh exist */
		{ 
			return newfname;
		}
		strcpy(&newfname[len],"hxx");
		if(file_is_regular(newfname))      /* test if *.hxx exist */
		{ 
			return newfname;
		}
		strcpy(&newfname[len],"hpp");
		if(file_is_regular(newfname))      /* test if *.hpp exist */
		{ 
			return newfname;
		}
	}
	g_free(newfname);	
	return NULL;
}
gboolean
move_file_if_not_same (gchar* src, gchar* dest)
{
	gboolean same;

	g_return_val_if_fail (src != NULL, FALSE);
	g_return_val_if_fail (dest != NULL, FALSE);

	if (anjuta_is_installed ("cmp", FALSE) == TRUE)
	{
		pid_t pid;
		gint status;
		if ((pid=fork())==0)
		{
			execlp ("cmp", "cmp", "-s", src, dest, NULL);
			g_error ("Cannot execute cmp");
		}
		waitpid (pid, &status, 0);
		if (WEXITSTATUS(status)==0)
			same = TRUE;
		else
			same = FALSE;
	}
	else
	{
		same = FALSE;
	}
	/* rename () can't work with 2 different filesystems, so we need
	 to use the couple {copy, remove}. Meanwhile, the files to copy
	 are supposed to be small, so it would spend more time to try 
	 rename () to know if a copy/deletion is needed */
	
	if (!same && !copy_file (src, dest, FALSE))
		return FALSE;
	
	remove (src);
	return TRUE;
}

gchar*
get_file_as_buffer (gchar* filename)
{
	struct stat s;
	gchar *buff;
	gint ret;
	FILE* fp;

	g_return_val_if_fail ( filename != NULL, NULL);
	ret = stat (filename, &s);
	if (ret != 0)
		return NULL;
	fp = fopen(filename, "r");
	if (!fp)
		return NULL;
	buff = g_malloc (s.st_size+3);
	ret = fread (buff, 1, s.st_size, fp);
	fclose (fp);
	buff[ret] = '\0';
	/* Error is not checked whether all the file is read or not. */
	/* Can be checked with ferror() on return from this function */
	return buff;
}

GList*
scan_files_in_dir (const char *dir, int (*select)(const struct dirent *))
{
	struct dirent **namelist;
	GList *files;
	int n, i;

	g_return_val_if_fail (dir != NULL, NULL);
	
	n = scandir (dir, &namelist, select, alphasort);
	files = NULL;
	if (n >= 0)
	{
		for (i = 0; i < n; i++)
			files = g_list_append (files, g_strdup (namelist[i]->d_name));
		free (namelist);
	}
	return files;
}

gchar*
string_from_glist (GList* list)
{
	GList* node;
	gchar* str;
	if (!list) return NULL;
	
	str = g_strdup("");
	node = list;
	while (node)
	{
		if (node->data)
		{
			gchar* tmp;
			tmp = str;
			str = g_strconcat (tmp, node->data, " ", NULL);
			g_free (tmp);
		}
		node = g_list_next (node);
	}
	if (strlen(str) == 0)
	{
		g_free (str);
		return NULL;
	}
	return str;
}
int
select_only_file (const struct dirent *e)
{
	return file_is_regular (e->d_name);
}

/* Create a new hbox with an image and a label packed into it
        * and return the box. */
GtkWidget*
create_xpm_label_box( GtkWidget *parent,
                                 const gchar     *xpm_filename, gboolean gnome_pixmap,
                                 const gchar     *label_text )
       {
           GtkWidget *box1;
           GtkWidget *label;
           GtkWidget *pixmap;

           /* Create box for xpm and label */
           box1 = gtk_hbox_new (FALSE, 0);

           /* Now on to the xpm stuff */
           pixmap = anjuta_res_get_pixmap_widget (parent, xpm_filename, gnome_pixmap);

           /* Create a label for the button */
           label = gtk_label_new (label_text);

           /* Pack the pixmap and label into the box */
           gtk_box_pack_start (GTK_BOX (box1),
                               pixmap, FALSE, FALSE, 0);

           gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 3);

           gtk_widget_show(pixmap);
           gtk_widget_show(label);

           return(box1);
}

/* Excluding the final 0 */
gint calc_string_len( const gchar *szStr )
{
	if( NULL == szStr )
		return 0;
	return strlen( szStr )*3 ;	/* Leave space for the translated character */
}

gint calc_gnum_len(void)
{
	return 24 ;	/* size of a stringfied integer */
}
/* Allocates a struct of pointers */
gchar **string_parse_separator( const gint nItems, gchar *szStrIn, const gchar chSep )
{
	gchar	**szAllocPtrs = (char**)g_new( gchar*, nItems );
	if( NULL != szAllocPtrs )
	{
		int			i ;
		gboolean	bOK = TRUE ;
		gchar		*p = szStrIn ;
		for( i = 0 ; i < nItems ; i ++ )
		{
			gchar		*szp ;
			szp = strchr( p, chSep ) ;
			if( NULL != szp )
			{
				szAllocPtrs[i] = p ;
				szp[0] = '\0' ;	/* Parse Operation */
				p = szp + 1 ;
			} else
			{
				bOK = FALSE ;
				break;
			}
		}
		if( ! bOK )
		{
			g_free( szAllocPtrs );
			szAllocPtrs = NULL ;
		}
	}
	return szAllocPtrs ;
}


/* Write in file....*/
gchar *WriteBufUL( gchar* szDst, const gulong ulVal)
{
	return szDst += sprintf( szDst, "%lu,", ulVal ); 
}

gchar *WriteBufI( gchar* szDst, const gint iVal )
{
	return szDst += sprintf( szDst, "%d,", iVal );
}

gchar *WriteBufB( gchar* szDst, const gboolean bVal )
{
	return szDst += sprintf( szDst, "%d,", (int)bVal );
}


gchar *WriteBufS( gchar* szDst, const gchar* szVal )
{
	if( NULL == szVal )
	{
		return szDst += sprintf( szDst, "," );
	} else
	{
		/* Scrive il valore convertendo eventuali caratteri non alfanumerici */
		/*int		nLen = strlen( szVal );*/
		const gchar	*szSrc = szVal ;
		while( szSrc[0] )
		{
			if( isalnum( szSrc[0] ) || ('.'==szSrc[0]) || ('/'==szSrc[0]) || ('_'==szSrc[0]) )
			{
				*szDst++ = *szSrc ;
			} else if( '\\' == szSrc[0] )
			{
				*szDst++ = *szSrc ;
				*szDst++ = *szSrc ;
			} else
			{
				szDst += sprintf( szDst, "\\%2.2X", (0x00FF&szSrc[0]) );
			}
			szSrc++ ;
		}
		*szDst++ = ',' ;
		*szDst = '\0' ;	}
	return szDst ;
}

#define	SRCH_CHAR	'\\'

static int GetHexAs( const gchar c )
{
	if( isdigit( c ) )
		return c - '0' ;
	else
		return toupper(c) - 'A' + 10 ;
}

static gchar GetHexb( const gchar c1, const gchar c2 )
{	return GetHexAs( c1 ) * 16 + GetHexAs( c2 ) ;
}

gchar* GetStrCod( const gchar *szIn )
{
	gchar	*szRet ;
	g_return_val_if_fail( NULL != szIn, NULL );
	szRet = g_malloc( strlen( szIn )+2 );
	if( NULL != szRet )
	{
		gchar*	szDst = szRet ;
		while( szIn[0] )
		{
			if( SRCH_CHAR == szIn[0] )
			{
				if( SRCH_CHAR == szIn[1] )
				{
					*szDst++ = *szIn ++ ;
					szIn ++ ;
				} else
				{
					*szDst ++ = GetHexb( szIn[1], szIn[2] ) ;
					szIn += 3;
				}
			} else
			{
				*szDst++ = *szIn ++ ;
			}
		}
		szDst [0] = '\0' ;
	}
	return szRet ;
}

