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
#define PREF_CALLTIP_ENABLE "language.cpp.code.calltip.enable"
#define MAX_COMPLETIONS 10
#define BRACE_SEARCH_LIMIT 500

G_DEFINE_TYPE (CppJavaAssist, cpp_java_assist, G_TYPE_OBJECT);

struct _CppJavaAssistPriv {
	AnjutaPreferences *preferences;
	IAnjutaSymbolManager* isymbol_manager;
	IAnjutaEditorAssist* iassist;
	
	/* Last used cache */
	gchar *search_cache;
	gchar *scope_context_cache;
	GCompletion *completion_cache;
};

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
	DEBUG_PRINT ("Iter column: %d", offset);
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

static GCompletion*
create_completion (IAnjutaEditorAssist* iassist, IAnjutaIterable* iter)
{
	GCompletion *completion = g_completion_new (NULL);
	GHashTable *suggestions_hash = g_hash_table_new (g_str_hash, g_str_equal);
	GList* suggestions = NULL;
	do
	{
		const gchar* name = ianjuta_symbol_name (IANJUTA_SYMBOL(iter), NULL);
		if (name != NULL)
		{
			if (!g_hash_table_lookup (suggestions_hash, name))
			{
				g_hash_table_insert (suggestions_hash, (gchar*)name,
									 (gchar*)name);
				suggestions = g_list_prepend (suggestions, g_strdup (name));
			}
		}
		else
			break;
	}
	while (ianjuta_iterable_next (iter, NULL));
	
	g_hash_table_destroy (suggestions_hash);
	
	suggestions = g_list_sort (suggestions, (GCompareFunc) strcmp);
	g_completion_add_items (completion, suggestions);
	return completion;
}

static gchar*
cpp_java_assist_get_scope_context (IAnjutaEditor* editor,
								   const gchar *scope_operator,
								   IAnjutaIterable *iter)
{
	gint end;
	gint begin;
	gchar ch;
	
	end = ianjuta_iterable_get_position (iter, NULL) + 1;
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);		
	while (ch && is_scope_context_character (ch))
	{
		if (!ianjuta_iterable_previous (iter, NULL))
			break;
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);		
	}
	
	begin = ianjuta_iterable_get_position (iter, NULL) + 1;
	if (end > begin)	
		return ianjuta_editor_get_text (editor, begin, end - begin, NULL);
	else
		return NULL;
}

static gchar*
cpp_java_assist_get_pre_word (IAnjutaEditor* editor, IAnjutaIterable *iter)
{
	gint end;
	gint begin;
	gchar ch;
	
	end = ianjuta_iterable_get_position (iter, NULL) + 1;
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	
	DEBUG_PRINT ("Looking for preword from pos: %d", end - 1);
	
	while (ch && is_word_character (ch))
	{
		if (!ianjuta_iterable_previous (iter, NULL))
			break;
		ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);		
	}
	
	begin = ianjuta_iterable_get_position (iter, NULL) + 1;
	
	if (end > begin)	
		return ianjuta_editor_get_text (editor, begin, end - begin, NULL);
	else
		return NULL;
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
cpp_java_assist_destroy_completion_cache (CppJavaAssist *assist)
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
			g_list_foreach (items, (GFunc) g_free, NULL);
			g_completion_clear_items (assist->priv->completion_cache);
		}
		g_completion_free (assist->priv->completion_cache);
		assist->priv->completion_cache = NULL;
	}
}

static void
cpp_java_assist_create_scope_completion_cache (CppJavaAssist *assist,
											   const gchar *scope_operator,
											   const gchar *scope_context)
{
	cpp_java_assist_destroy_completion_cache (assist);
	if (g_str_equal (scope_operator, "::"))
	{
		IAnjutaIterable* iter = 
			ianjuta_symbol_manager_get_members (assist->priv->isymbol_manager,
												scope_context, TRUE, NULL);
		if (iter)
		{
			assist->priv->completion_cache =
				create_completion (assist->priv->iassist, iter);
			assist->priv->scope_context_cache = g_strdup (scope_context);
			g_object_unref (iter);
		}
	}
	else if (g_str_equal (scope_operator, ".") ||
			 g_str_equal (scope_operator, "->"))
	{
		/* TODO: Find the type of context by parsing the file somehow and
		search for the member as it is done with the :: context */
	}
}

static void
cpp_java_assist_create_word_completion_cache (CppJavaAssist *assist,
											  const gchar *pre_word)
{
	cpp_java_assist_destroy_completion_cache (assist);
	IAnjutaIterable* iter = 
		ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
										IANJUTA_SYMBOL_TYPE_MAX,
										pre_word, TRUE, TRUE, NULL);
	if (iter)
	{
		assist->priv->completion_cache =
			create_completion (assist->priv->iassist, iter);
		assist->priv->search_cache = g_strdup (pre_word);
		g_object_unref (iter);
	}
}

static gboolean
cpp_java_assist_show_autocomplete (CppJavaAssist *assist,
								   const gchar *pre_word)
{
	gint position, max_completions, length;
	GList *completion_list;
	
	g_return_val_if_fail (assist->priv->completion_cache != NULL, FALSE);
	
	if (pre_word)
		g_completion_complete (assist->priv->completion_cache, pre_word, NULL);
	else
		g_completion_complete (assist->priv->completion_cache, "", NULL);

	position =
		ianjuta_editor_get_position (IANJUTA_EDITOR (assist->priv->iassist),
									 NULL);
	max_completions =
		anjuta_preferences_get_int_with_default (assist->priv->preferences,
												 PREF_AUTOCOMPLETE_CHOICES,
												 MAX_COMPLETIONS);
	if (assist->priv->completion_cache->cache)
		completion_list = assist->priv->completion_cache->cache;
	else
		completion_list = assist->priv->completion_cache->items;
	
	length = g_list_length (completion_list);
	if (length <= max_completions)
	{
		if (length > 1 || !pre_word || !g_str_equal (pre_word,
													 completion_list->data))
		{
			ianjuta_editor_assist_suggest (assist->priv->iassist,
										   completion_list,
										   position,
										   pre_word? strlen (pre_word) : 0,
										   NULL);
			return TRUE;
		}
	}
	return FALSE;
}

static gchar*
cpp_java_assist_get_calltip_context (CppJavaAssist *assist,
									 IAnjutaIterable *iter,
									 gint *context_offset)
{
	gchar ch;
	gchar *context = NULL;
	
#ifdef DEBUG
	GTimer* timer = g_timer_new ();
#endif
	
	ch = ianjuta_editor_cell_get_char (IANJUTA_EDITOR_CELL (iter), 0, NULL);
	if (ch == ')')
	{
		if (!cpp_java_util_jump_to_matching_brace (iter, ')', -1))
			return NULL;
		if (!ianjuta_iterable_previous (iter, NULL))
			return NULL;
		DEBUG_PRINT ("calltip ')' brace: %f", g_timer_elapsed (timer, NULL));
	}
	if (ch != '(')
	{
		if (!cpp_java_util_jump_to_matching_brace (iter, ')',
												   BRACE_SEARCH_LIMIT))
			return NULL;
		DEBUG_PRINT ("calltip ')' brace: %f", g_timer_elapsed (timer, NULL));
	}
	
	/* Skip white spaces */
	while (ianjuta_iterable_previous (iter, NULL)
		&& g_ascii_isspace (ianjuta_editor_cell_get_char
								(IANJUTA_EDITOR_CELL (iter), 0, NULL)));

	DEBUG_PRINT ("calltip skip whitespace: %f", g_timer_elapsed (timer, NULL));
	
	context = cpp_java_assist_get_scope_context
		(IANJUTA_EDITOR (assist->priv->iassist), "(", iter);
	
	DEBUG_PRINT ("calltip get scope context: %f", g_timer_elapsed (timer, NULL));
	
	if (context_offset)
	{
		*context_offset = get_iter_column (assist, iter);
		DEBUG_PRINT ("calltip get iter column: %f", g_timer_elapsed (timer, NULL));
	}

#ifdef DEBUG
	g_timer_destroy (timer);
#endif
	
	return context;
}

static gboolean
cpp_java_assist_show_calltip (CppJavaAssist *assist, gchar *call_context,
							  gint context_offset,
							  IAnjutaIterable *position_iter)
{
	GList *tips = NULL;
	
	IAnjutaIterable* iter = 
		ianjuta_symbol_manager_search (assist->priv->isymbol_manager,
									   IANJUTA_SYMBOL_TYPE_PROTOTYPE|
									   IANJUTA_SYMBOL_TYPE_FUNCTION|
									   IANJUTA_SYMBOL_TYPE_METHOD|
									   IANJUTA_SYMBOL_TYPE_MACRO_WITH_ARG,
									   call_context, FALSE, TRUE, NULL);
	if (iter)
	{
		do
		{
			IAnjutaSymbol* symbol = IANJUTA_SYMBOL(iter);
			const gchar* name = ianjuta_symbol_name(symbol, NULL);
			if (name != NULL)
			{
				const gchar* args = ianjuta_symbol_args(symbol, NULL);
				gchar* print_args;
				gchar* separator;
				gchar* white_name = g_strnfill (strlen(name) + 1, ' ');
				
				separator = g_strjoin (NULL, ", \n", white_name, NULL);
				DEBUG_PRINT ("Separator: \n%s", separator);
				
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
		
		if (tips)
		{
			/* Calculate calltip offset from context offset */
			gint position = ianjuta_iterable_get_position (position_iter, NULL);
			gint char_alignment =
				get_iter_column (assist, position_iter)- context_offset;
			
			if (char_alignment < 0)
				char_alignment = context_offset;
			
			ianjuta_editor_assist_show_tips (assist->priv->iassist, tips,
											 position + 1, char_alignment,
											 NULL);
			g_list_foreach (tips, (GFunc) g_free, NULL);
			g_list_free (tips);
			return TRUE;
		}
	}
	return FALSE;
}

gboolean
cpp_java_assist_check (CppJavaAssist *assist, gboolean autocomplete,
					   gboolean calltips)
{
	gint position;
	gboolean shown = FALSE;
	IAnjutaEditor *editor;
	IAnjutaIterable *iter, *iter_save;
	IAnjutaEditorAttribute attribute;
	gchar *pre_word = NULL, *scope_operator = NULL;
#ifdef DEBUG
	GTimer* timer = g_timer_new();
#endif
	
	DEBUG_PRINT ("Autocomplete enable is: %d", autocomplete);
	DEBUG_PRINT ("Calltips enable is: %d", calltips);
	
	if (!autocomplete && !calltips)
		return FALSE; /* Nothing to do */
	
	editor = IANJUTA_EDITOR (assist->priv->iassist);
	
	position = ianjuta_editor_get_position (editor, NULL);
	iter = ianjuta_editor_get_cell_iter (editor, position, NULL);
	ianjuta_iterable_previous (iter, NULL);
	iter_save = ianjuta_iterable_clone (iter, NULL);
	
	attribute = ianjuta_editor_cell_get_attribute (IANJUTA_EDITOR_CELL (iter),
												   NULL);
	/*
	if (attribute == IANJUTA_EDITOR_COMMENT ||
		attribute == IANJUTA_EDITOR_STRING);
	{
		g_object_unref (iter);
		g_object_unref (iter_save);
		return FALSE;
	}
	*/
	
	DEBUG_PRINT ("assist init: %f", g_timer_elapsed (timer, NULL));
	
	if (autocomplete)
	{
		pre_word = cpp_java_assist_get_pre_word (editor, iter);
		DEBUG_PRINT ("assist pre word: %f", g_timer_elapsed (timer, NULL));
		
		scope_operator = cpp_java_assist_get_scope_operator (editor, iter);
		DEBUG_PRINT ("assist scope operator: %f", g_timer_elapsed (timer, NULL));
		
		DEBUG_PRINT ("Pre word: %s", pre_word);
		DEBUG_PRINT ("Scope op: %s", scope_operator);
		
		if (scope_operator)
		{
			gchar *scope_context = NULL;
			scope_context = cpp_java_assist_get_scope_context (editor,
															   scope_operator,
															   iter);
			
			DEBUG_PRINT ("assist scope context: %f", g_timer_elapsed (timer, NULL));
			DEBUG_PRINT ("Scope context: %s", scope_context);
			
			if (scope_context)
			{
				if (!assist->priv->scope_context_cache ||
					strcmp (scope_context,
							assist->priv->scope_context_cache) != 0)
				{
					cpp_java_assist_create_scope_completion_cache (assist,
																   scope_operator,
																   scope_context);
				}
				shown = cpp_java_assist_show_autocomplete (assist, pre_word);
			}
			g_free (scope_context);
		}
		else if (pre_word && strlen (pre_word) > 3)
		{
			if (!assist->priv->search_cache ||
				strncmp (assist->priv->search_cache,
						 pre_word, strlen (assist->priv->search_cache)) != 0)
			{
				cpp_java_assist_create_word_completion_cache (assist, pre_word);
			}
			shown = cpp_java_assist_show_autocomplete (assist, pre_word);
		}
		if (!shown)
		{
			ianjuta_editor_assist_hide_suggestions (assist->priv->iassist,
													NULL);
		}
		DEBUG_PRINT ("assist autocomplete: %f", g_timer_elapsed (timer, NULL));
	}
#ifdef DEBUG
	g_timer_reset (timer);
#endif
	if (calltips)
	{
		if (!shown)
		{
			gint offset;
			gchar *call_context =
				cpp_java_assist_get_calltip_context (assist, iter, &offset);
			DEBUG_PRINT ("get calltip context: %f", g_timer_elapsed (timer, NULL));
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
		DEBUG_PRINT ("assist calltip: %f", g_timer_elapsed (timer, NULL));
	}
	g_object_unref (iter);
	g_object_unref (iter_save);
	g_free (pre_word);
	g_free (scope_operator);
#ifdef DEBUG
	g_timer_stop (timer);
	g_timer_destroy (timer);
#endif
	return shown;
}

static void
on_editor_char_added (IAnjutaEditor *editor, gint insert_pos, gchar ch,
					  CppJavaAssist *assist)
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
on_assist_chosen (IAnjutaEditorAssist* iassist, gint selection,
				  CppJavaAssist* assist)
{
	gint cur_pos;
	const gchar* assistance;
	IAnjutaEditor *te;
	IAnjutaIterable *iter;
	gchar *pre_word = NULL;
	
	DEBUG_PRINT ("assist-chosen: %d", selection);
	
	if (assist->priv->completion_cache->cache)
		assistance = g_list_nth_data (assist->priv->completion_cache->cache,
									  selection);
	else
		assistance = g_list_nth_data (assist->priv->completion_cache->items,
									  selection);
	
	te = IANJUTA_EDITOR (assist->priv->iassist);
	cur_pos = ianjuta_editor_get_position (te, NULL);
	iter = ianjuta_editor_get_cell_iter (te, cur_pos, NULL);
	
	if (ianjuta_iterable_previous (iter, NULL))
	{
		pre_word = cpp_java_assist_get_pre_word (te, iter);
	}
	ianjuta_document_begin_undo_action (IANJUTA_DOCUMENT (te), NULL);
	if (pre_word)
	{
		gint sel_start;
		sel_start = ianjuta_iterable_get_position (iter, NULL);
		ianjuta_editor_selection_set (IANJUTA_EDITOR_SELECTION (te),
									  sel_start + 1, cur_pos, FALSE, NULL);
		ianjuta_editor_selection_replace (IANJUTA_EDITOR_SELECTION (te),
										  assistance, -1, NULL);
		g_free (pre_word);
	}
	else
	{
		ianjuta_editor_insert (te, cur_pos, assistance, -1, NULL);
	}
	ianjuta_document_end_undo_action (IANJUTA_DOCUMENT (te), NULL);
	ianjuta_editor_assist_hide_suggestions (assist->priv->iassist, NULL);
	g_object_unref (iter);
}

static void
cpp_java_assist_install (CppJavaAssist *assist, IAnjutaEditorAssist *iassist)
{
	DEBUG_PRINT(__FUNCTION__);
	
	assist->priv->iassist = iassist;
	g_signal_connect (iassist, "char-added",
					  G_CALLBACK (on_editor_char_added), assist);
	g_signal_connect (G_OBJECT(iassist), "assist-chosen",
					  G_CALLBACK(on_assist_chosen), assist);
}

static void
cpp_java_assist_uninstall (CppJavaAssist *assist)
{
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK(on_assist_chosen), assist);
	g_signal_handlers_disconnect_by_func (assist->priv->iassist,
										  G_CALLBACK (on_editor_char_added),
										  assist);
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
	cpp_java_assist_destroy_completion_cache (assist);
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
