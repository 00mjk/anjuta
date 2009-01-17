/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * cpp-java-assist.c
 * Copyright (C)  2007 Naba Kumar  <naba@gnome.org>
 *                     Johannes Schmid  <jhs@gnome.org>
 * 
 * anjuta is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * anjuta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with anjuta.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <ctype.h>
#include <string.h>
#include <libanjuta/anjuta-debug.h>
#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-selection.h>
#include <libanjuta/interfaces/ianjuta-document.h>
#include <libanjuta/interfaces/ianjuta-symbol-manager.h>
#include "cpp-java-assist.h"
#include "cpp-java-utils.h"

#define PREF_AUTOCOMPLETE_ENABLE "language.cpp.code.completion.enable"
#define PREF_AUTOCOMPLETE_CHOICES "language.cpp.code.completion.choices"
#define PREF_AUTOCOMPLETE_SPACE_AFTER_FUNC "language.cpp.code.completion.space.after.func"
#define PREF_AUTOCOMPLETE_BRACE_AFTER_FUNC "language.cpp.code.completion.brace.after.func"
#define PREF_CALLTIP_ENABLE "language.cpp.code.calltip.enable"
#define MAX_COMPLETIONS 10
#define BRACE_SEARCH_LIMIT 500

G_DEFINE_TYPE (CppJavaAssist, cpp_java_assist, G_TYPE_OBJECT);

typedef struct
{
	gchar *name;
	gboolean is_func;
	IAnjutaSymbolType type;
} CppJavaAssistTag;

struct _CppJavaAssistPriv {
	AnjutaPreferences *preferences;
	IAnjutaSymbolManager* isymbol_manager;
	IAnjutaEditorAssist* iassist;
	
	/* Last used cache */
	gchar *search_cache;
	gchar *scope_context_cache;
	gchar *pre_word;
	GCompletion *completion_cache;
	gboolean editor_only;
	guint word_idle;
};

static gchar*
completion_function (gpointer data)
{
	CppJavaAssistTag * tag = (CppJavaAssistTag*) data;
	return tag->name;
}

static gint
completion_compare (gconstpointer a, gconstpointer b)
{
	CppJavaAssistTag * tag_a = (CppJavaAssistTag*) a;
	CppJavaAssistTag * tag_b = (CppJavaAssistTag*) b;
	return (strcmp (tag_a->name, tag_b->name) &&
			tag_a->type == tag_b->type);
}

static void
cpp_java_assist_tag_destroy (CppJavaAssistTag *tag)
{
	g_free (tag->name);
	g_free (tag);
}

static gint
get_iter_column (CppJavaAssist *assist, IAnjutaIterable *iter)
{
	gchar ch;
	gint offset = 0;
	gint tabsize =
		ianjuta_editor_get_tabsize (IANJUTA_EDITOR (assist->priv->iassist),
									NULL);
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
									   0, NULL);
	
	while (ch != '\n')
	{
		if (!ianjuta_iterable_previous (iter, NULL))
			break;
		if (ch == '\t')
			offset += tabsize - 1;
		offset++;
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
										   0, NULL);
	}
	//DEBUG_PRINT ("Iter column: %d", offset);
	return offset;
}

static gboolean
is_scope_context_character (gchar ch)
{
	if (g_ascii_isspace (ch))
		return FALSE;
	if (g_ascii_isalnum (ch))
		return TRUE;
	if (ch == '_' || ch == '.' || ch == ':' || ch == '>' || ch == '-')
		return TRUE;
	
	return FALSE;
}	

static gboolean
is_word_character (gchar ch)
{
	if (g_ascii_isspace (ch))
		return FALSE;
	if (g_ascii_isalnum (ch))
		return TRUE;
	if (ch == '_')
		return TRUE;
	
	return FALSE;
}	

/**
 * If mergeable is NULL than no merge will be made with iter elements, elsewhere
 * mergeable will be returned with iter elements.
 */
static GCompletion*
create_completion (IAnjutaEditorAssist* iassist, IAnjutaIterable* iter,
				   GCompletion* mergeable)
{	
	GCompletion *completion;
	
	if (mergeable == NULL)
		completion = g_completion_new (completion_function);
	else
		completion = mergeable;
	
	GList* suggestions = NULL;
	do
	{
		const gchar* name = ianjuta_symbol_get_name (IANJUTA_SYMBOL(iter), NULL);
		if (name != NULL)
		{
			CppJavaAssistTag *tag = g_new0 (CppJavaAssistTag, 1);
			tag->name = g_strdup (name);
			tag->type = ianjuta_symbol_get_sym_type (IANJUTA_SYMBOL (iter),
													 NULL);
			tag->is_func = (tag->type == IANJUTA_SYMBOL_TYPE_FUNCTION ||
							tag->type == IANJUTA_SYMBOL_TYPE_METHOD ||
							tag->type == IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG);
			if (!g_list_find_custom (suggestions, tag, completion_compare))
				suggestions = g_list_prepend (suggestions, tag);
			else
				g_free (tag);
		}
		else
			break;
	}
	while (ianjuta_iterable_next (iter, NULL));
	
	
	
	suggestions = g_list_sort (suggestions, completion_compare);
	g_completion_add_items (completion, suggestions);
	return completion;
}

#define SCOPE_BRACE_JUMP_LIMIT 50

static gchar*
cpp_java_assist_get_scope_context (IAnjutaEditor* editor,
								   const gchar *scope_operator,
								   IAnjutaIterable *iter)
{
	IAnjutaIterable* end;
	gchar ch, *scope_chars = NULL;
	gboolean out_of_range = FALSE;
	gboolean scope_chars_found = FALSE;
	
	end = ianjuta_iterable_clone (iter, NULL);
	ianjuta_iterable_next (end, NULL);
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	
	while (ch)
	{
		if (is_scope_context_character (ch))
		{
			scope_chars_found = TRUE;
		}
		else if (ch == ')')
		{
			if (!cpp_java_util_jump_to_matching_brace (iter, ch, SCOPE_BRACE_JUMP_LIMIT))
			{
				out_of_range = TRUE;
				break;
			}
		}
		else
			break;
		if (!ianjuta_iterable_previous (iter, NULL))
		{
			out_of_range = TRUE;
			break;
		}		
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	}
	if (scope_chars_found)
	{
		IAnjutaIterable* begin;
		begin = ianjuta_iterable_clone (iter, NULL);
		if (!out_of_range)
			ianjuta_iterable_next (begin, NULL);
		scope_chars = ianjuta_editor_get_text (editor, begin, end, NULL);
		g_object_unref (begin);
	}
	g_object_unref (end);
	return scope_chars;
}

static gchar*
cpp_java_assist_get_pre_word (IAnjutaEditor* editor, IAnjutaIterable *iter)
{
	IAnjutaIterable *end;
	gchar ch, *preword_chars = NULL;
	gboolean out_of_range = FALSE;
	gboolean preword_found = TRUE;
	
	end = ianjuta_iterable_clone (iter, NULL);
	ianjuta_iterable_next (end, NULL);

	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	
	while (ch && is_word_character (ch))
	{
		preword_found = TRUE;
		if (!ianjuta_iterable_previous (iter, NULL))
		{
			out_of_range = TRUE;
			break;
		}
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	}
	
	if (preword_found)
	{
		IAnjutaIterable *begin = ianjuta_iterable_clone (iter, NULL);
		if (!out_of_range)
			ianjuta_iterable_next (begin, NULL);
		preword_chars = ianjuta_editor_get_text (editor, begin, end, NULL);
		g_object_unref (begin);
	}
	g_object_unref (end);
	return preword_chars;
}

static gchar*
cpp_java_assist_get_scope_operator (IAnjutaEditor* editor,
									IAnjutaIterable *iter)
{
	gchar op[3] = {'\0', '\0', '\0'};
	
	op[1] = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	if (op[1] == ':' || op[1] == '>' || op[1] == '.')
	{
		if (ianjuta_iterable_previous (iter, NULL))
		{
			op[0] = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter),
												  0, NULL);
			if ((op[0] == ':' && op[1] == ':') ||
				(op[0] == '-' && op[1] == '>'))
			{
				ianjuta_iterable_previous (iter, NULL);
				return g_strdup (op);
			}
			else
			{
				if (op[1] == '.')
					return g_strdup (&op[1]);
			}
		}
		else
		{
			if (op[1] == '.')
				return g_strdup (&op[1]);
		}
	}
	return NULL;
}

static void
cpp_java_assist_destroy_completion_cache (CppJavaAssist *assist,
										  gboolean cancel_idle)
{
	if (assist->priv->search_cache)
	{
		g_free (assist->priv->search_cache);
		assist->priv->search_cache = NULL;
	}
	if (assist->priv->scope_context_cache)
	{
		g_free (assist->priv->scope_context_cache);
		assist->priv->scope_context_cache = NULL;
	} 
	if (assist->priv->completion_cache)
	{
		GList* items = assist->priv->completion_cache->items;
		if (items)
		{
			g_list_foreach (items, (GFunc) cpp_java_assist_tag_destroy, NULL);
			g_completion_clear_items (assist->priv->completion_cache);
		}
		g_completion_free (assist->priv->completion_cache);
		assist->priv->completion_cache = NULL;
	}
	if (assist->priv->word_idle > 0 && cancel_idle)
	{
		g_source_remove (assist->priv->word_idle);
		assist->priv->word_idle = 0;
	}
}

static void
cpp_java_assist_create_scope_completion_cache (CppJavaAssist *assist,
											   const gchar *scope_operator,
											   const gchar *scope_context)
{		
	DEBUG_PRINT ("scope context: %s", scope_context);
	cpp_java_assist_destroy_completion_cache (assist, TRUE);
	if (g_str_equal (scope_operator, "::"))
	{
		/* Go through the possible namespace (Gnome::Glade, for example) */
		GStrv contexts = g_strsplit (scope_context, "::", -1);
		gint cur_context = 0;
		if (contexts[0] != NULL)
		{
			DEBUG_PRINT ("Scope[%d] = %s", cur_context, contexts[0]);
			IAnjutaIterable* symbol = 
				ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
											   IANJUTA_SYMBOL_TYPE_CLASS | IANJUTA_SYMBOL_TYPE_NAMESPACE,
											   TRUE,
											   IANJUTA_SYMBOL_FIELD_SIMPLE,
											   contexts[0],
											   FALSE,
											   TRUE,
											   TRUE,
											   1,
											   -1,
											   NULL);
			if (symbol && ianjuta_iterable_get_length(symbol, NULL))
			{
				while (contexts[++cur_context] != NULL)
				{							
					DEBUG_PRINT ("Scope[%d] = %s", cur_context, contexts[0]);
					IAnjutaIterable* members = 
						ianjuta_symbol_manager_get_members(assist->priv->isymbol_manager,
														   IANJUTA_SYMBOL(symbol),
														   IANJUTA_SYMBOL_FIELD_SIMPLE,
														   TRUE, NULL);
					if (members && ianjuta_iterable_get_length (members, NULL))
					{
						gboolean found = FALSE;
						do
						{
							if (g_str_equal (ianjuta_symbol_get_name (IANJUTA_SYMBOL(members),
																	  NULL),
											 contexts[cur_context]))
							{
								g_object_unref (symbol);
								symbol = members;
								found = TRUE;
								break;
							}
						}
						while (ianjuta_iterable_next (members, NULL));
						if (found)
							continue;
						else
						{
							g_object_unref (symbol);
							symbol = NULL;
							break;
						}
					}
				}
				if (symbol)
				{
					IAnjutaIterable* members = 
						ianjuta_symbol_manager_get_members(assist->priv->isymbol_manager,
														   IANJUTA_SYMBOL(symbol),
														   IANJUTA_SYMBOL_FIELD_SIMPLE,
														   TRUE, NULL);
					if (members)
					{
						assist->priv->completion_cache =
							create_completion (assist->priv->iassist, members, NULL);
						assist->priv->scope_context_cache = g_strdup (scope_context);
						g_object_unref (members);
					}
					g_object_unref (symbol);
				}
			}
		}
		g_strfreev(contexts);
	}
	
	else if (g_str_equal (scope_operator, ".") ||
			 g_str_equal (scope_operator, "->"))	
	{
		/* TODO: Find the type of context by parsing the file somehow and
		search for the member as it is done with the :: context */
	}	
}

static gboolean
cpp_java_assist_show_autocomplete (CppJavaAssist *assist)
{
	IAnjutaIterable *position;
	gint max_completions, length;
	GList *completion_list;

	if (assist->priv->completion_cache == NULL) return FALSE;	
	
	if (assist->priv->pre_word)
		g_completion_complete (assist->priv->completion_cache, assist->priv->pre_word, NULL);
	else
		g_completion_complete (assist->priv->completion_cache, "", NULL);

	position =
		ianjuta_editor_get_position (IANJUTA_EDITOR (assist->priv->iassist),
									 NULL);
	max_completions =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_CHOICES,
												 MAX_COMPLETIONS);
	/* If there is cache use that */
	if (assist->priv->completion_cache->cache)
		completion_list = assist->priv->completion_cache->cache;
	
	/* If there is no cache, it means that no string completion happened
	 * because the list is being shown for member completion just after
	 * scope operator where there is no preword yet entered. So use the
	 * full list because that's the full list of members of that scope.
	 */
	else if (!assist->priv->pre_word)
		completion_list = assist->priv->completion_cache->items;
		
	else
		return FALSE;
	
	length = g_list_length (completion_list);
	if (length <= max_completions)
	{
		if (length > 1 || !assist->priv->pre_word ||
			!g_str_equal (assist->priv->pre_word,
						  ((CppJavaAssistTag*)completion_list->data)->name))
		{
			GList *node, *suggestions = NULL;
			gint alignment;
			
			node = completion_list;
			while (node)
			{
				CppJavaAssistTag *tag = node->data;
				
				gchar *entry;
				
				if (tag->is_func)
					entry = g_strdup_printf ("%s()", tag->name);
				else
					entry = g_strdup_printf ("%s", tag->name);
				suggestions = g_list_prepend (suggestions, entry);
				node = g_list_next (node);
			}
			suggestions = g_list_reverse (suggestions);
			alignment = assist->priv->pre_word? strlen (assist->priv->pre_word) : 0;
			
			ianjuta_editor_assist_suggest (assist->priv->iassist,
										   suggestions,
										   position,
										   alignment,
										   NULL);
			g_list_foreach (suggestions, (GFunc) g_free, NULL);
			g_list_free (suggestions);
			g_object_unref (position);
			return TRUE;
		}
	}
	g_object_unref (position);
	return FALSE;
}

static gboolean
cpp_java_assist_create_word_completion_cache (CppJavaAssist *assist)
{
	gint max_completions;
	GCompletion *completion = NULL;
	GList* editor_completions = NULL;
	gboolean shown = FALSE;

	max_completions =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_CHOICES,
												 MAX_COMPLETIONS);
	
	cpp_java_assist_destroy_completion_cache (assist, FALSE);
	
	if (!assist->priv->editor_only)
	{
		IAnjutaIterable* iter_project = 
			ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
										   IANJUTA_SYMBOL_TYPE_MAX,
										   TRUE,
										   IANJUTA_SYMBOL_FIELD_SIMPLE|IANJUTA_SYMBOL_FIELD_TYPE,
										   assist->priv->pre_word, TRUE, FALSE, FALSE, -1, -1, NULL);
		
		IAnjutaIterable* iter_globals = 
			ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
										   IANJUTA_SYMBOL_TYPE_MAX,
										   TRUE,
										   IANJUTA_SYMBOL_FIELD_SIMPLE|IANJUTA_SYMBOL_FIELD_TYPE,
										   assist->priv->pre_word, TRUE, TRUE, TRUE, -1, -1, NULL);
		
		if (iter_project) 
		{
			completion = create_completion (assist->priv->iassist, iter_project, NULL);
			g_object_unref (iter_project);
		}
		
		if (iter_globals)
		{
			
			completion = create_completion (assist->priv->iassist, iter_globals, completion);
			g_object_unref (iter_globals);
		}
	}
	editor_completions = ianjuta_editor_assist_get_suggestions (assist->priv->iassist,
																assist->priv->pre_word,
																NULL);
	if (editor_completions)
	{
		GList* tag_list = NULL;
		GList* node;
		for (node = editor_completions; node != NULL; node = g_list_next (node))
		{
			CppJavaAssistTag *tag = g_new0 (CppJavaAssistTag, 1);
			tag->name = node->data;
			tag->type = 0;
			tag->is_func = FALSE;
			if (completion && !g_list_find_custom (completion->items, tag, 
												   completion_compare))
				tag_list = g_list_append (tag_list, tag);
			else
				cpp_java_assist_tag_destroy (tag);
		}
		if (!completion)
		{
			completion = g_completion_new(completion_function);
			assist->priv->editor_only = TRUE;
		}
		else
			assist->priv->editor_only = FALSE;

		tag_list = g_list_sort (tag_list, completion_compare);
		g_completion_add_items (completion, tag_list);		
		g_list_free (editor_completions);
	}
	
	assist->priv->completion_cache = completion;
	assist->priv->search_cache = g_strdup (assist->priv->pre_word);
	
	shown = cpp_java_assist_show_autocomplete (assist);
	if (!shown)
		ianjuta_editor_assist_hide_suggestions (assist->priv->iassist,
												NULL);
	DEBUG_PRINT ("Show autocomplete: %d", shown);
	
	return FALSE;
}

static gchar*
cpp_java_assist_get_calltip_context (CppJavaAssist *assist,
									 IAnjutaIterable *iter,
									 gint *context_offset)
{
	gchar ch;
	gchar *context = NULL;
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	if (ch == ')')
	{
		if (!cpp_java_util_jump_to_matching_brace (iter, ')', -1))
			return NULL;
		if (!ianjuta_iterable_previous (iter, NULL))
			return NULL;
	}
	if (ch != '(')
	{
		if (!cpp_java_util_jump_to_matching_brace (iter, ')',
												   BRACE_SEARCH_LIMIT))
			return NULL;
	}
	
	/* Skip white spaces */
	while (ianjuta_iterable_previous (iter, NULL)
		&& g_ascii_isspace (ianjuta_editor_cell_get_char
								(IANJUTA_EDITOR_CELL (iter), 0, NULL)));

	
	context = cpp_java_assist_get_scope_context
		(IANJUTA_EDITOR (assist->priv->iassist), "(", iter);
	
	if (context_offset)
	{
		*context_offset = get_iter_column (assist, iter);
	}
	
	return context;
}

static GList*
cpp_java_assist_create_calltips (IAnjutaIterable* iter)
{
	GList* tips = NULL;
	if (iter)
	{
		do
		{
			IAnjutaSymbol* symbol = IANJUTA_SYMBOL(iter);
			const gchar* name = ianjuta_symbol_get_name(symbol, NULL);
			if (name != NULL)
			{
				const gchar* args = ianjuta_symbol_get_args(symbol, NULL);
				gchar* print_args;
				gchar* separator;
				gchar* white_name = g_strnfill (strlen(name) + 1, ' ');
				
				separator = g_strjoin (NULL, ", \n", white_name, NULL);
				//DEBUG_PRINT ("Separator: \n%s", separator);
				
				gchar** argv;
				if (!args)
					args = "()";
				
				argv = g_strsplit (args, ",", -1);
				print_args = g_strjoinv (separator, argv);
				
				gchar* tip = g_strdup_printf ("%s %s", name, print_args);
				
				if (!g_list_find_custom (tips, tip, (GCompareFunc) strcmp))
					tips = g_list_append (tips, tip);
				
				g_strfreev (argv);
				g_free (print_args);
				g_free (separator);
				g_free (white_name);
			}
			else
				break;
		}
		while (ianjuta_iterable_next (iter, NULL));
	}
	return tips;
}

static gboolean
cpp_java_assist_show_calltip (CppJavaAssist *assist, gchar *call_context,
							  gint context_offset,
							  IAnjutaIterable *position_iter)
{	
	GList *tips = NULL;
	gint max_completions;
	
	max_completions =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_CHOICES,
												 MAX_COMPLETIONS);
	/* Search global */
	IAnjutaIterable* iter_global = 
		ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
									   IANJUTA_SYMBOL_TYPE_PROTOTYPE|
									   IANJUTA_SYMBOL_TYPE_FUNCTION|
									   IANJUTA_SYMBOL_TYPE_METHOD|
									   IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
									   TRUE, IANJUTA_SYMBOL_FIELD_SIMPLE,
									   call_context, FALSE, TRUE, TRUE,
									   max_completions, -1, NULL);
	tips = cpp_java_assist_create_calltips (iter_global);
	/* Search Project */
	IAnjutaIterable* iter_project = 
		ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
									   IANJUTA_SYMBOL_TYPE_PROTOTYPE|
									   IANJUTA_SYMBOL_TYPE_FUNCTION|
									   IANJUTA_SYMBOL_TYPE_METHOD|
									   IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
									   TRUE, IANJUTA_SYMBOL_FIELD_SIMPLE,
									   call_context, FALSE, TRUE, FALSE,
									   max_completions, -1, NULL);
	tips = g_list_concat (tips, cpp_java_assist_create_calltips (iter_project));
	if (iter_global)
		g_object_unref (iter_global);
	if (iter_project)
		g_object_unref (iter_project);
	if (tips)
	{
		/* Calculate calltip offset from context offset */
		gint char_alignment =
			get_iter_column (assist, position_iter)- context_offset;
		
		if (char_alignment < 0)
			char_alignment = context_offset;
		
		ianjuta_editor_assist_show_tips (assist->priv->iassist, tips,
										 position_iter, char_alignment,
										 NULL);
		g_list_foreach (tips, (GFunc) g_free, NULL);
		g_list_free (tips);
		return TRUE;
	}
	return FALSE;
}

gboolean
cpp_java_assist_check (CppJavaAssist *assist, gboolean autocomplete,
					   gboolean calltips)
{
	gboolean shown = FALSE;
	IAnjutaEditor *editor;
	IAnjutaIterable *iter, *iter_save;	
	
	if (!autocomplete && !calltips)
		return FALSE; /* Nothing to do */
	
	editor = IANJUTA_EDITOR (assist->priv->iassist);
	
	iter = ianjuta_editor_get_position (editor, NULL);
	ianjuta_iterable_previous (iter, NULL);
	iter_save = ianjuta_iterable_clone (iter, NULL);
	
	if (autocomplete)
	{
		gboolean shown = FALSE;
		g_free (assist->priv->pre_word);
		assist->priv->pre_word = cpp_java_assist_get_pre_word (editor, iter);
		DEBUG_PRINT ("Pre word: %s", assist->priv->pre_word);
		
		if (assist->priv->pre_word && strlen (assist->priv->pre_word) > 3)
		{
			if (!assist->priv->search_cache ||
				!g_str_has_prefix (assist->priv->pre_word, assist->priv->search_cache))
			{
				g_idle_add_full (G_PRIORITY_LOW,
								 (GSourceFunc) cpp_java_assist_create_word_completion_cache,
								 assist,
								 NULL);
				DEBUG_PRINT ("Idle source added");
			}
		}
		shown = cpp_java_assist_show_autocomplete (assist);
		if (!shown)
			ianjuta_editor_assist_hide_suggestions (assist->priv->iassist,
													NULL);
		DEBUG_PRINT ("Show autocomplete: %d", shown);
	}
	if (calltips)
	{
		if (!shown)
		{
			gint offset;
			gchar *call_context =
				cpp_java_assist_get_calltip_context (assist, iter, &offset);
			if (call_context)
			{
				shown = cpp_java_assist_show_calltip (assist, call_context,
													  offset, iter_save);
			}
			else
			{
				ianjuta_editor_assist_cancel_tips (assist->priv->iassist, NULL);
			}
			g_free (call_context);
		}
	}
	
	g_object_unref (iter);
	g_object_unref (iter_save);
	
	return shown;
}

static void
on_editor_char_added (IAnjutaEditor *editor, IAnjutaIterable *insert_pos,
					  gchar ch, CppJavaAssist *assist)
{
	gboolean enable_complete =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_ENABLE,
												 TRUE);
	
	gboolean enable_calltips =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_CALLTIP_ENABLE,
												 TRUE);
	cpp_java_assist_check (assist, enable_complete, enable_calltips);
}

static void
on_editor_backspace (IAnjutaEditor* editor, CppJavaAssist* assist)
{
	on_editor_char_added (editor, NULL, '\b', assist);
}

static void
on_assist_chosen (IAnjutaEditorAssist* iassist, gint selection,
				  CppJavaAssist* assist)
{
	CppJavaAssistTag *tag;
	IAnjutaIterable *cur_pos;
	GString *assistance;
	IAnjutaEditor *te;
	IAnjutaIterable *iter;
	gchar *pre_word = NULL;
	gboolean add_space_after_func = FALSE;
	gboolean add_brace_after_func = FALSE;
	
	//DEBUG_PRINT ("assist-chosen: %d", selection);
	
	if (assist->priv->completion_cache->cache)
		tag = g_list_nth_data (assist->priv->completion_cache->cache,
							   selection);
	else
		tag = g_list_nth_data (assist->priv->completion_cache->items,
							   selection);
	
	assistance = g_string_new (tag->name);
	
	if (tag->is_func)
	{
		add_space_after_func =
			anjuta_preferences_get_int_with_default (assist->priv->preferences,
													 PREF_AUTOCOMPLETE_SPACE_AFTER_FUNC,
													 TRUE);
		add_brace_after_func =
			anjuta_preferences_get_int_with_default (assist->priv->preferences,
													 PREF_AUTOCOMPLETE_BRACE_AFTER_FUNC,
													 TRUE);
		if (add_space_after_func)
			g_string_append (assistance, " ");
		
		if (add_brace_after_func)
			g_string_append (assistance, "(");
	}
	
	te = IANJUTA_EDITOR (assist->priv->iassist);
	cur_pos = ianjuta_editor_get_position (te, NULL);
	iter = ianjuta_iterable_clone (cur_pos, NULL);
	
	if (ianjuta_iterable_previous (iter, NULL))
	{
		pre_word = cpp_java_assist_get_pre_word (te, iter);
	}
	
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (te), NULL);
	if (pre_word)
	{
		ianjuta_iterable_next (iter, NULL);
		ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (te),
									  iter, cur_pos, NULL);
		ianjuta_editor_selection_replace (IANJUTA_EDITOR_SELECTION (te),
										  assistance->str, -1, NULL);
		g_free (pre_word);
	}
	else
	{
		ianjuta_editor_insert (te, cur_pos, assistance->str, -1, NULL);
	}
	g_object_unref (iter);
	g_object_unref (cur_pos);

	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (te), NULL);
	
	ianjuta_editor_assist_hide_suggestions (assist->priv->iassist, NULL);
	
	/* Show calltip if we completed function */
	if (add_brace_after_func)
		cpp_java_assist_check (assist, FALSE, TRUE);
	
	g_string_free (assistance, TRUE);
}

static void
cpp_java_assist_install (CppJavaAssist *assist, IAnjutaEditorAssist *iassist)
{
	g_return_if_fail (assist->priv->iassist == NULL);
	
	assist->priv->iassist = iassist;
	g_signal_connect (iassist, "char-added",
					  G_CALLBACK (on_editor_char_added), assist);
	g_signal_connect (iassist, "backspace",
					  G_CALLBACK (on_editor_backspace), assist);
	g_signal_connect (iassist, "assist-chosen",
					  G_CALLBACK(on_assist_chosen), assist);
}

static void
cpp_java_assist_uninstall (CppJavaAssist *assist)
{
	g_return_if_fail (assist->priv->iassist != NULL);
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK(on_assist_chosen), assist);
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK (on_editor_char_added),
										  assist);
 g_signal_handlers_disconnect_by_func (assist->priv->iassist,

  G_CALLBACK (on_editor_backspace), assist);

	assist->priv->iassist = NULL;
}

static void
cpp_java_assist_init (CppJavaAssist *assist)
{
	assist->priv = g_new0 (CppJavaAssistPriv, 1);
}

static void
cpp_java_assist_finalize (GObject *object)
{
	CppJavaAssist *assist = CPP_JAVA_ASSIST (object);
	cpp_java_assist_uninstall (assist);
	cpp_java_assist_destroy_completion_cache (assist, TRUE);
	g_free (assist->priv);
	G_OBJECT_CLASS (cpp_java_assist_parent_class)->finalize (object);
}

static void
cpp_java_assist_class_init (CppJavaAssistClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = cpp_java_assist_finalize;
}

CppJavaAssist *
cpp_java_assist_new (IAnjutaEditorAssist *iassist,
					 IAnjutaSymbolManager *isymbol_manager,
					 AnjutaPreferences *prefs)
{
	CppJavaAssist *assist = g_object_new (TYPE_CPP_JAVA_ASSIST, NULL);
	assist->priv->isymbol_manager = isymbol_manager;
	assist->priv->preferences = prefs;
	cpp_java_assist_install (assist, iassist);
	return assist;
}
