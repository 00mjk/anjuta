/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

/*
** search-replace_backend.c: Generic Search and Replace
** Author: Biswapesh Chattopadhyay
*/

/*
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gnome.h>
#include <glade/glade.h>
#include <libgnomevfs/gnome-vfs.h>

#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-shell.h>
#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/anjuta-encodings.h>
#include <libanjuta/anjuta-convert.h>
#include <libanjuta/interfaces/ianjuta-editor.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-file.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-project-manager.h>

/*
#define GTK
#undef PLAT_GTK
#define PLAT_GTK 1
#include "Scintilla.h"
#include "SciLexer.h"
#include "ScintillaWidget.h"
*/

#include "search-replace_backend.h"
#include "tm_tagmanager.h"

/* Information about a matched substring */
typedef struct _MatchSubStr
{
	gint start;
	gint len;
} MatchSubStr;

static SearchReplace *sr = NULL;

void clear_search_replace_instance(void);

static void match_substr_free(MatchSubStr *ms)
{
	g_free(ms);
}


void
match_info_free (MatchInfo *mi)
{
	if (mi)
	{
		if (mi->subs)
		{
			GList *tmp;
			for (tmp = mi->subs; tmp; tmp = g_list_next(tmp))
				match_substr_free((MatchSubStr *) tmp->data);
			g_list_free(mi->subs);
		}
		g_free(mi);
	}
}


void
file_buffer_free (FileBuffer *fb)
{
	if (fb)
	{
		if (fb->path)
			g_free(fb->path);
		if (fb->uri)
			g_free (fb->uri);
		if (fb->buf)
			g_free(fb->buf);
		if (fb->lines)
			g_list_free(fb->lines);
		g_free(fb);
	}
}

/* Create a file buffer structure from a TextEditor structure */
FileBuffer *
file_buffer_new_from_te (IAnjutaEditor *te)
{
	FileBuffer *fb;
	GFile* file;
	gchar* path;
	
	g_return_val_if_fail(te, NULL);
	fb = g_new0(FileBuffer, 1);
	fb->type = FB_EDITOR;
	fb->te = te;
	
	file = ianjuta_file_get_file(IANJUTA_FILE(te), NULL);
	path = g_file_get_path (file);
	if (path)
	{
		fb->path = tm_get_real_path(path);
		g_free (path);
	}
	fb->uri = g_file_get_uri (file);
	fb->len = ianjuta_editor_get_length(te, NULL);
	fb->buf = ianjuta_editor_get_text_all (fb->te, NULL);
	fb->pos = ianjuta_editor_get_offset(fb->te, NULL);
	fb->line = ianjuta_editor_get_lineno(fb->te, NULL);
	
	g_object_unref (file);
	
	return fb;
}

/* Only use the first 500 chars for validating (yes, I feel lucky...) */
#define MAX_VALIDATE 500

FileBuffer *
file_buffer_new_from_path (const char *path, const char *buf, int len, int pos)
{
	FileBuffer *fb;
	IAnjutaEditor *te;
	IAnjutaDocument* doc;
	char *real_path;
	GFile* file;
	char *uri;
	int i;
	int lineno;

	g_return_val_if_fail(path, NULL);
	real_path = tm_get_real_path(path);
	
	/* There might be an already open TextEditor with this path */
	file = g_file_new_for_path (real_path);
	uri = g_file_get_uri (file);
	doc = ianjuta_document_manager_find_document_with_file (sr->docman,
														 file, NULL);
	g_object_unref (file);

	if (doc && IANJUTA_IS_EDITOR (doc))
	{
		te = IANJUTA_EDITOR (doc);
		g_free(real_path);
		return file_buffer_new_from_te(te);
	}
	fb = g_new0(FileBuffer, 1);
	fb->type = FB_FILE;
	fb->path = real_path;
	fb->uri = uri;
	fb->name = strrchr(path, '/');
	if (fb->name)
		++ fb->name;
	else
		fb->name = fb->path;
	if (buf && len > 0)
	{
		fb->buf = g_new(char, len + 1);
		memcpy(fb->buf, buf, len);
		fb->buf[len] = '\0';
		fb->len = len;
	}
	else
	{
		struct stat s;

		if ((0 == stat(fb->path, &s)) && (S_ISREG(s.st_mode)))
		{
			if ((fb->len = s.st_size) < 0) return NULL;
			fb->buf = g_new(char, s.st_size + 1);
			{
				int total_bytes = 0, bytes_read, fd;
				if (0 > (fd = open(fb->path, O_RDONLY)))
				{
					perror(fb->path);
					file_buffer_free(fb);
					return NULL;
				}
				while (total_bytes < s.st_size)
				{
					if (0 > (bytes_read = read(fd, fb->buf + total_bytes
					  , s.st_size - total_bytes)))
					{
						perror(fb->path);
						close(fd);
						file_buffer_free(fb);
						return NULL;
					}
					total_bytes += bytes_read;
				}
				close(fd);
				fb->buf[fb->len] = '\0';
			}
		}
	}
	if (!g_utf8_validate (fb->buf, MIN(MAX_VALIDATE, fb->len), NULL))
	{
		const AnjutaEncoding *encoding_used = NULL;
		gchar* converted_text;
		guint converted_len;
		converted_text = anjuta_convert_to_utf8 (fb->buf, 
												 fb->len, 
												 &encoding_used,
												 &converted_len, 
												 NULL);
		if (converted_text == NULL)
		{
			/* Last change, let's try 8859-15 */
			encoding_used =  
				anjuta_encoding_get_from_charset("ISO-8859-15");
			
			converted_text = anjuta_convert_to_utf8 (fb->buf,
												  fb->len,
												  &encoding_used,
												  &converted_len,
												  NULL);
		}
		if (converted_text == NULL)
		{
			/* Give up */
			file_buffer_free(fb);
			return NULL;
		}
		else
		{
			g_free (fb->buf);
			fb->buf = converted_text;
			fb->len = converted_len;
		}
	}
	
	if (pos <= 0 || pos > fb->len)
	{
		fb->pos = 0;
		fb->line = 0;
	}
	else
	{
		fb->pos = pos;
		fb->line = 0;
	}
	/* First line starts at column 0 */
	fb->lines = g_list_prepend(fb->lines, GINT_TO_POINTER(0));
	lineno = 0;
	for (i=0; i < fb->len; ++i)
	{
		if ('\n' == fb->buf[i] && '\0' != fb->buf[i+1])
		{
			fb->lines = g_list_prepend(fb->lines, GINT_TO_POINTER(i + 1));
			if (0 == fb->line && fb->pos > i)
				fb->line = lineno;
			++ lineno;
		}
	}
	fb->lines = g_list_reverse(fb->lines);
	return fb;
}

static long
file_buffer_line_from_pos(FileBuffer *fb, int pos)
{
	GList *tmp;
	int lineno = -1;
	g_return_val_if_fail(fb && pos >= 0, 1);
	if (FB_FILE == fb->type)
	{
		for (tmp = fb->lines; tmp; tmp = g_list_next(tmp))
		{
			if (pos < GPOINTER_TO_INT(tmp->data))
				return lineno;
			++ lineno;
		}
		return lineno;
	}
	else if (FB_EDITOR == fb->type)
	{
		IAnjutaIterable *position;
		position = ianjuta_editor_get_position_from_offset (fb->te, pos, NULL);
		lineno = ianjuta_editor_get_line_from_position (fb->te, position, NULL);
		g_object_unref (position);
		return lineno;
	}
	else
		return -1;
}

gchar *
file_match_line_from_pos(FileBuffer *fb, int pos)
{
	gint length=1;
	gint i;
	g_return_val_if_fail(fb && pos >= 0, NULL);

	for (i= pos+1; ((fb->buf[i] != '\n') && (fb->buf[i] != '\0')); i++, length++);
	for (i= pos-1; (fb->buf[i] != '\n') && (i >= 0); i--, length++);
	
	return g_strndup (fb->buf + i + 1, length);
}

/* Generate a list of files to search in. Call with start = TRUE and
** top_dir = sf->top_dir. This is used when the search range is specified as
SR_FILES */
static GList *
create_search_files_list(SearchFiles *sf, const char *top_dir)
{
	TMFileEntry *entry;
	GList *files;

	g_return_val_if_fail(sf && top_dir, NULL);
	entry = tm_file_entry_new(top_dir, NULL, sf->recurse, sf->match_files
	  , sf->ignore_files, sf->match_dirs, sf->ignore_dirs
	  , sf->ignore_hidden_files, sf->ignore_hidden_dirs);
	if (!entry)
		return NULL;
	files = tm_file_entry_list(entry, NULL);
	tm_file_entry_free(entry);
	return files;
}

/* Get a list of all project files */
static GList *
get_project_file_list(void)
{
	GList* list = NULL;
	GList *files = NULL;
	gchar *project_root_uri = NULL;
	
	anjuta_shell_get (ANJUTA_PLUGIN(sr->docman)->shell,
					  "project_root_uri", G_TYPE_STRING,
					  &project_root_uri, NULL);
	
	if (project_root_uri)
	{
		IAnjutaProjectManager* prjman;
		prjman = anjuta_shell_get_interface(ANJUTA_PLUGIN(sr->docman)->shell, 
											IAnjutaProjectManager , NULL);
		
		list = ianjuta_project_manager_get_elements (prjman,
													 IANJUTA_PROJECT_MANAGER_SOURCE,
													 NULL);
		if (list)
		{
			const gchar *uri;
			GList *node;
			node = list;
	
			while (node)
			{
				gchar *file_path;
		
				uri = (const gchar *)node->data;
				file_path = gnome_vfs_get_local_path_from_uri (uri);
				if (file_path)
				files = g_list_prepend (files, file_path);
				node = g_list_next (node);
			}
			files = g_list_reverse (files);
			g_list_free(list);
		}
	}
	g_free (project_root_uri);
	return files;
}


static gboolean
isawordchar (gunichar c)
{
	return (g_unichar_isalnum(c) || '_' == c);
}

static gboolean
extra_match (FileBuffer *fb, gchar* begin, gchar* end, SearchExpression *s)
{
	gunichar b, e;
	
	b = g_utf8_get_char (g_utf8_prev_char (begin));
	e = g_utf8_get_char (end);
	
	if (s->whole_line)
		if ((fb->pos == 0 || b == '\n' || b == '\r') &&
			(e == '\0'	|| e == '\n' || e == '\r'))
			return TRUE;
		else
			return FALSE;
	else if (s->whole_word)
		if ((fb->pos ==0 || !isawordchar(b)) && 
			(e=='\0' || !isawordchar(e)))
			return TRUE;
		else
			return FALSE;
	else if (s->word_start)
		if (fb->pos ==0 || !isawordchar(b))
			return TRUE;
		else
			return FALSE;	
	else
		return TRUE;
}

/* Returns the next match in the passed buffer. The search expression should
   be pre-compiled. The returned pointer should be freed with match_info_free()
   when no longer required. */
MatchInfo *
get_next_match(FileBuffer *fb, SearchDirection direction, SearchExpression *s)
{
	MatchInfo *mi = NULL;

	g_return_val_if_fail(fb && s, NULL);
	
	if (s->regex)
	{
		GMatchInfo* match_info;
		if (s->regex_info == NULL)
		{
			GError* error = NULL;
			GRegexCompileFlags compile_flags = 0;
			GRegexMatchFlags match_flags = 0;
		
			match_flags |= G_REGEX_MATCH_NOTEMPTY;
			if (s->ignore_case)
			{
				compile_flags |= G_REGEX_CASELESS;
			}
			if (!s->greedy)
			{
				compile_flags |= G_REGEX_UNGREEDY;
			}
			s->regex_info = g_regex_new (s->search_str, compile_flags,
										 match_flags, &error);
			if (error)
			{
				anjuta_util_dialog_error (NULL, error->message);
				g_error_free(error);
				s->regex_info = NULL;
				return NULL;
			}
		}

		g_regex_match_full (s->regex_info, fb->buf, fb->len,
					   g_utf8_offset_to_pointer (fb->buf, fb->pos) - fb->buf, 
					   G_REGEX_MATCH_NOTEMPTY, &match_info, NULL);
		
		if (g_match_info_matches (match_info))
		{
			gint start;
			gint end;
			int i;
			mi = g_new0(MatchInfo, 1);
			if (g_match_info_fetch_pos (match_info, 0, &start, &end))
			{
				DEBUG_PRINT ("Regex: %d %d", start, end);
				mi->pos = g_utf8_pointer_to_offset (fb->buf, fb->buf + start);
				mi->len = g_utf8_pointer_to_offset (fb->buf, fb->buf + end) - mi->pos;
				mi->line = file_buffer_line_from_pos(fb, mi->pos);
			}
			for (i = 1; i < g_match_info_get_match_count(match_info); i++) /* Captured subexpressions */
			{
				MatchSubStr *ms;
				ms = g_new0(MatchSubStr, 1);
				if (g_match_info_fetch_pos (match_info, i, &start, &end))
				{
					ms->start = g_utf8_pointer_to_offset (fb->buf, fb->buf + start);
					ms->len = g_utf8_pointer_to_offset (fb->buf, fb->buf + end) - ms->start;
				}
				mi->subs = g_list_prepend(mi->subs, ms);
			}
			mi->subs = g_list_reverse(mi->subs);
			fb->pos = g_utf8_pointer_to_offset (fb->buf, fb->buf + end);
		}
	}
	else
	{
		/* Simple string search - this needs to be performance-tuned */
		gboolean found;
		gint match_len;

		match_len = strlen (s->search_str);
		if (match_len == 0)
			return NULL;

		found = FALSE;
		if (SD_BACKWARD == direction)
		{
			/* Backward matching. */
			if (s->ignore_case)
			{
				gchar* current = g_utf8_offset_to_pointer (fb->buf, fb->pos);
				gint len = g_utf8_strlen (s->search_str, -1);
				gchar* search_caseless = g_utf8_casefold (s->search_str, len);
				for (; fb->pos >= len; --fb->pos)
				{
					gchar* current_caseless = g_utf8_casefold (current, len);
					if (g_utf8_collate (current_caseless, search_caseless) == 0 &&
						extra_match (fb, current, current + strlen (search_caseless),
									 s))
					{
						found = TRUE;
						g_free (current_caseless);
						break;
					}
					else
						current = g_utf8_prev_char (current);
				}
				g_free (search_caseless);
			}
			else
			{
				gchar* current = g_utf8_offset_to_pointer (fb->buf, fb->pos);
				gint len = g_utf8_strlen (s->search_str, -1);
				gchar* search_key = g_utf8_collate_key (s->search_str, len);
				for (; fb->pos >= len; --fb->pos)
				{
					gchar* current_key = g_utf8_collate_key (current, len);
					if (g_str_equal (current_key, search_key) &&
						extra_match (fb, current, current + strlen (s->search_str),
									 s))
					{
						found = TRUE;
						g_free (current_key);
						break;
					}
					else
						current = g_utf8_prev_char (current);
				}
				g_free (search_key);
			}
		}
		else
		{
			/* Forward match */
			if (s->ignore_case)
			{
				gchar* current = g_utf8_offset_to_pointer (fb->buf, fb->pos);
				gint len = g_utf8_strlen (s->search_str, -1);
				gchar* search_caseless = g_utf8_casefold (s->search_str, len);
				gint buf_len = g_utf8_strlen (fb->buf, fb->len);
				for (; fb->pos < buf_len; ++fb->pos)
				{
					gchar* current_caseless = g_utf8_casefold (current, len);
					if (g_utf8_collate (current_caseless, search_caseless) == 0 &&
						extra_match (fb, current, current + strlen (search_caseless),
									 s))
					{
						found = TRUE;
						g_free (current_caseless);
						break;
					}
					else
						current = g_utf8_next_char (current);
				}
				g_free (search_caseless);
			}
			else
			{
				gchar* current = g_utf8_offset_to_pointer (fb->buf, fb->pos);
				gint len = g_utf8_strlen (s->search_str, -1);
				gint buf_len = g_utf8_strlen (fb->buf, fb->len);
				gchar* search_key = g_utf8_collate_key (s->search_str, len);
				for (; fb->pos < buf_len; ++fb->pos)
				{
					gchar* current_key = g_utf8_collate_key (current, len);
					if (g_str_equal (current_key, search_key) &&
						extra_match (fb, current, current + strlen (s->search_str),
									 s))
					{
						found = TRUE;
						g_free (current_key);
						break;
					}
					else
						current = g_utf8_next_char (current);
				}
				g_free (search_key);
			}
		}
		if (found)
		{
			mi = g_new0 (MatchInfo, 1);	//better to abort than silently fail to report match ?
//			mi = g_try_new0 (MatchInfo, 1);
//			if (mi)
//			{
				mi->pos = fb->pos;
				mi->len = match_len;
				mi->line = file_buffer_line_from_pos (fb, fb->pos);
//			}
//			else
//				WARN USER ABOUT MEMORY ERROR
			if (direction == SD_BACKWARD)
				fb->pos -= match_len;
			else
				fb->pos += match_len;
		}
	}
	return mi;
}

/* Create list of search entries */
GList *
create_search_entries (Search *s)
{
	GList *entries = NULL;
	GList *tmp;
	GList *editors;
	IAnjutaDocument *doc;
	SearchEntry *se;
	gint tmp_pos;
	gint selstart;

	switch (s->range.type)
	{
		case SR_BUFFER:
			doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
			if (doc && IANJUTA_IS_EDITOR (doc))
			{
				se = g_new0 (SearchEntry, 1);
				se->type = SE_BUFFER;
				se->te = IANJUTA_EDITOR (doc);
				se->direction = s->range.direction;
				if (SD_BEGINNING == se->direction)
				{
					se->start_pos = 0;
					se->end_pos = -1;
					se->direction = SD_FORWARD;
				}
				else
				{	
					IAnjutaIterable *start;
					/* forward-search from after beginning of selection, if any
					   backwards-search from before beginning of selection, if any
					   treat -ve positions except -1 as high +ve */
					start = ianjuta_editor_selection_get_start
							(IANJUTA_EDITOR_SELECTION (se->te), NULL);
					if (start)
					{
						selstart =
							ianjuta_iterable_get_position (start, NULL);
						if (se->direction == SD_BACKWARD)
						{
							se->start_pos = (selstart != 0) ?
								 selstart - 1 : selstart;
						}
						else
						{
							se->start_pos =
								(selstart != -2 &&
								 selstart < ianjuta_editor_get_length (IANJUTA_EDITOR (se->te), NULL)) ?
								 selstart + 1 : selstart;
						}
						g_object_unref (start);
					}
					else
					{
						se->start_pos = ianjuta_editor_get_offset (se->te, NULL);
					}
					se->end_pos = -1;	/* not actually used when backward searching */
				}
				entries = g_list_prepend(entries, se);
			}
			break;
		case SR_SELECTION:
		case SR_BLOCK:
		case SR_FUNCTION: 
			doc = ianjuta_document_manager_get_current_document (sr->docman, NULL);
			if (doc && IANJUTA_IS_EDITOR (doc))
			{
				gint selend;
				
				se = g_new0 (SearchEntry, 1);
				se->type = SE_BUFFER;
				se->te = IANJUTA_EDITOR (doc);
				se->direction = s->range.direction;
				if (se->direction == SD_BEGINNING)
					se->direction = SD_FORWARD;

				if (s->range.type == SR_SELECTION)
				{
					selstart = selend = 0;	/* warning prevention only */
				}				
				else
				{
					IAnjutaIterable* end =
						ianjuta_editor_selection_get_end (IANJUTA_EDITOR_SELECTION (se->te), NULL);
					if (end)
					{
						selstart = selend = ianjuta_iterable_get_position (end, NULL);
						g_object_unref (end);
					}
					else
					{
						selstart = selend = 0;	/* warning prevention only */
						g_assert ("No selection end position");
					}
				}

				if (s->range.type == SR_BLOCK)
					ianjuta_editor_selection_select_block(IANJUTA_EDITOR_SELECTION (se->te), NULL);
				if (s->range.type == SR_FUNCTION)
					ianjuta_editor_selection_select_function(IANJUTA_EDITOR_SELECTION (se->te), NULL);
				{
					IAnjutaIterable *start, *end;
					start = ianjuta_editor_selection_get_start (IANJUTA_EDITOR_SELECTION (se->te), NULL);
					end = ianjuta_editor_selection_get_end(IANJUTA_EDITOR_SELECTION (se->te), NULL);
					se->start_pos =  ianjuta_iterable_get_position (start, NULL);
					se->end_pos = ianjuta_iterable_get_position (end, NULL);
					g_object_unref (start);
					g_object_unref (end);
				}
				if (se->direction == SD_BACKWARD)
				{
					tmp_pos = se->start_pos;
					se->start_pos = se->end_pos;
					se->end_pos = tmp_pos;
				}	
				entries = g_list_prepend (entries, se);
				if (s->range.type != SR_SELECTION)
				{
					IAnjutaIterable *start, *end;
					start = ianjuta_editor_get_position_from_offset (se->te, selstart, NULL);
					end = ianjuta_editor_get_position_from_offset (se->te, selend, NULL);
					ianjuta_editor_selection_set(IANJUTA_EDITOR_SELECTION (se->te), 
				                                 start, end, NULL);	
					g_object_unref (start);
					g_object_unref (end);
				}
			}
			break;
		case SR_OPEN_BUFFERS:
			editors = ianjuta_document_manager_get_doc_widgets (sr->docman, NULL);
			for (tmp = editors; tmp; tmp = g_list_next(tmp))
			{
				if (IANJUTA_IS_EDITOR (tmp->data))
				{
					se = g_new0 (SearchEntry, 1);
				se->type = SE_BUFFER;
					se->te = IANJUTA_EDITOR (tmp->data);
				se->direction = SD_FORWARD;
				se->start_pos = 0;
				se->end_pos = -1;
				entries = g_list_prepend(entries, se);
			}
			}
			entries = g_list_reverse(entries);
			g_list_free (editors);
			break;
		case SR_FILES:
		case SR_PROJECT:
		{
			GList *files = NULL;
			gchar *dir = NULL;
			gchar *dir_uri = NULL;		
					
			anjuta_shell_get (ANJUTA_PLUGIN(sr->docman)->shell,
							  "project_root_uri", G_TYPE_STRING,
							  &dir_uri, NULL);
			// FIXME : Replace Standard UNIX IO functions by gnome-vfs 
			if (dir_uri)			
				dir = gnome_vfs_get_local_path_from_uri(dir_uri);
			
			if (!dir)
			{
				if (SR_PROJECT == s->range.type)
					s->range.type = SR_FILES;
				dir = g_get_current_dir();
			}

			if (SR_FILES == s->range.type)
				files = create_search_files_list(&(s->range.files), dir);
			else /* if (SR_PROJECT == s->range.type) */
				files = get_project_file_list();	
			
			if (files)
			{
				for (tmp = files; tmp; tmp = g_list_next(tmp))
				{
					se = g_new0(SearchEntry, 1);
					se->type = SE_FILE;
					se->path = (char *) tmp->data;
					se->direction = SD_FORWARD;
					se->type = SE_FILE;
					se->start_pos = 0;
					se->end_pos = -1;
					entries = g_list_prepend(entries, se);
				}
				g_list_free(files);
				entries = g_list_reverse(entries);
			}
			g_free(dir);
			g_free(dir_uri);
			break;
		}
	}	
	return entries;		
}

gchar *
regex_backref (MatchInfo *mi, FileBuffer *fb)
{
#define REGX_BUFSIZE 1024
	gint i, j, k;
	gint nb_backref;
	gint i_backref;
	gint plen;
	gint start, len;
	gint backref[10] [2];	/* backref [0][2] unused */
	gchar buf [REGX_BUFSIZE + 4];	/* usable space + word-sized space for trailing 0 */
	GList *tmp;
	
	i = 1;
	/* Extract back references */
	tmp = mi->subs;
	while (tmp && i < 10)
	{
		backref[i] [0] = ((MatchSubStr*)tmp->data)->start;
		backref[i] [1] = ((MatchSubStr*)tmp->data)->len;
		tmp= g_list_next(tmp);
		i++;
	}
	nb_backref = i;
	plen = strlen (sr->replace.repl_str);
	for(i=0, j=0; i < plen && j < REGX_BUFSIZE; i++)
	{
		if (sr->replace.repl_str[i] == '\\')
		{
			i++;
			if (sr->replace.repl_str[i] > '0' && sr->replace.repl_str[i] <= '9')
			{
				i_backref = sr->replace.repl_str[i] - '0';
				if (i_backref < nb_backref)
				{
					start = backref[i_backref] [0];
					len = backref[i_backref] [1];
					for (k=0; k < len && j < REGX_BUFSIZE; k++)
						buf[j++] = fb->buf[start + k];	
				}
			}	
		}	
		else
			buf[j++] = sr->replace.repl_str[i];				
	}
	buf[j] = '\0';

	return g_strdup (buf);
}

#define FREE_FN(fn, v) if (v) { fn(v); v = NULL; }

void
clear_search_replace_instance(void)
{
	g_free (sr->search.expr.search_str);
	if (sr->search.expr.regex_info)
	{
		g_regex_unref (sr->search.expr.regex_info);
		sr->search.expr.regex_info = NULL;
	}
	if (SR_FILES == sr->search.range.type)
	{
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.match_files);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.ignore_files);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.match_dirs);
		FREE_FN(anjuta_util_glist_strings_free, sr->search.range.files.ignore_dirs);
	}
	FREE_FN(anjuta_util_glist_strings_free, sr->search.expr_history);
	g_free (sr->replace.repl_str);
	FREE_FN(anjuta_util_glist_strings_free, sr->replace.expr_history);
}

SearchReplace *
create_search_replace_instance(IAnjutaDocumentManager *docman)
{
	/* Create a new SearchReplace instance */
	if (NULL == sr)
	{
		sr = g_new0(SearchReplace, 1);
		sr->search.expr.regex_info = NULL;
	}
	else
		clear_search_replace_instance ();
	if (docman)
		sr->docman = docman;
	return sr;
}
