/*
 * sourceview-autocomplete.c (c) 2006 Johannes Schmid
 * Based on the Python-Code from Guillaume Chazarain
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "sourceview-autocomplete.h"
#include "sourceview-private.h"

#include <libanjuta/interfaces/ianjuta-editor.h> 
#include <libanjuta/anjuta-debug.h>
#include <pcre.h>

/* Maximal found autocompletition words */
const gint MAX_MATCHES = 5;
const gint N_MAX = 15; 

static gboolean
find_duplicate(GList* list, gchar* word)
{
	while(list)
	{
		if (strcmp(list->data, word) == 0)
			return TRUE;
		list = g_list_next(list);
	}
	return FALSE;
}


/* Find all words which start with the current word an put them in a List
	Return NULL if no or more then five occurences were found */

static GList*
get_completions(Sourceview* sv)
{
	GtkTextIter start_iter, end_iter;
	gchar* current_word = ianjuta_editor_get_current_word(IANJUTA_EDITOR(sv), NULL);
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
    gchar* text;
	pcre *re;
    GList* words = NULL;
    gchar* regex;
    gint err_offset;
	const gchar* err;
	gint rc;
	int ovector[N_MAX];

	
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);    
    text = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
    if (current_word == NULL && !strlen(current_word))
        return NULL;
	
	regex = g_strdup_printf("\\s%s\\w*\\s", current_word);
	re = pcre_compile(regex, 0, &err, &err_offset, NULL);
   	g_free(regex);
	
	if (NULL == re)
	{
		/* Compile failed - check error message */
		DEBUG_PRINT("Regex compile failed! %s at position %d", err, err_offset);
		return NULL;
	}
	
	ovector[1] = 0;
	for (;;)
	{
		int options = 0;                 /* Normally no options */
		int start_offset = ovector[1];   /* Start at end of previous match */
		gchar* comp_word;
		int start, end;
		
 	 	if (ovector[0] == ovector[1])
    	{
    		if (ovector[0] == strlen(text)) 
    			break;
    		options = PCRE_NOTEMPTY | PCRE_ANCHORED;
    	}

  		/* Run the next matching operation */
		rc = pcre_exec(re, NULL, text, strlen(text), start_offset,
					   0, ovector, N_MAX);

	 	if (rc == PCRE_ERROR_NOMATCH)
    	{
    		if (options == 0) 
    			break;
    		ovector[1] = start_offset + 1;
    		continue;
    	}

 		 if (rc < 0)
    	{
    		DEBUG_PRINT("Matching error %d\n", rc);
    		return NULL;
    	}

		if (rc == 0)
    	{
    		break;
    	}
    	
    	start = ovector[0];
    	end = ovector[1];
    	
    	if (start + 2 >= end)
    		continue;
    	
    	DEBUG_PRINT("Start: %d End: %d", start, end);
    	comp_word = g_new0(gchar, end - start + 1);
    	strncpy(comp_word, text + start + 1, end - start - 2);
    	DEBUG_PRINT("Completion found: %s", comp_word);
    	if (!find_duplicate(words, comp_word) &&
    		strcmp(comp_word, current_word) != 0)
		    words = g_list_append(words, comp_word);
    }
    g_free(text);   
    return words;
}

/* Return a tuple containing the (x, y) position of the cursor */
static void
get_coordinates(Sourceview* sv, int* x, int* y)
{
	int xor, yor;
	GdkRectangle rect;	
	GdkWindow* window;
	GtkTextIter cursor;
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextView* view = GTK_TEXT_VIEW(sv->priv->view);
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer)); 
	gtk_text_view_get_iter_location(view, &cursor, &rect);
	window = gtk_text_view_get_window(view, GTK_TEXT_WINDOW_TEXT);
	gtk_text_view_buffer_to_window_coords(view, GTK_TEXT_WINDOW_TEXT, 
		rect.x + rect.width, rect.y, x, y);
	
	gdk_window_get_origin(window, &xor, &yor);
	*x = *x + xor;
	*y = *y + yor;
}

/* Select to map callback of the Entry */

static void
select_to_map(GtkEntry* entry, GdkEvent* event, gchar* word)
{
	gtk_editable_select_region(GTK_EDITABLE(entry), strlen(word), -1);
}

/* Return an entry which contains all
	completion words */

static GtkWidget* 
make_entry_completion(Sourceview* sv)
{
	GtkTextBuffer* buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkWidget* entry;
	GtkEntryCompletion* completion;
	GtkListStore* list_store;
	gint max_word_length;	
	GList* words = get_completions(sv);
	GList* word;
	gchar* first_word;
	gchar* current_word;
	if (words == NULL || g_list_length(words) > 5)
		return NULL;
	
	entry = gtk_entry_new();
	completion = gtk_entry_completion_new();
	list_store = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(list_store));
	gtk_entry_completion_set_text_column(completion, 0);
	first_word = g_strdup(words->data);
	current_word = ianjuta_editor_get_current_word(IANJUTA_EDITOR(sv), NULL);
    word = words;
    while (word != NULL)
    {
        GtkTreeIter iter;
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter, 0, word->data, -1);
        //g_free(word->data);
        word = g_list_next(word);
     }
    gtk_entry_set_completion(GTK_ENTRY(entry), completion);
	gtk_entry_set_text(GTK_ENTRY(entry), first_word);
	gtk_entry_completion_set_popup_completion(completion, TRUE);
	g_signal_connect_after(G_OBJECT(entry), "map-event", G_CALLBACK(select_to_map), current_word);
	
	gtk_widget_show_all(entry);
	return entry;
}

/* Destroy dialog */

static gboolean
idle_destroy(gpointer* data)
{
    GtkWidget* dialog = GTK_WIDGET(data);
	gtk_widget_destroy(dialog);
	return FALSE;
}

static void 
activate_entry(GtkEntry* entry, GtkDialog* dialog)
{
	gtk_dialog_response(dialog, GTK_RESPONSE_OK);
	g_idle_add((GSourceFunc)idle_destroy, dialog);
}

static void
deactivate_entry(GtkEntry* entry, GtkDialog* dialog)
{
	g_idle_add((GSourceFunc)idle_destroy, dialog);
}

/* Return an undecorated dialog with the completion entry */

static const gchar*
get_word_replacement(Sourceview* sv)
{
	GtkWidget* entry = make_entry_completion(sv);
	GtkWidget* completion_dialog;
	gint x, y;
	gint response;
	if (entry == NULL)
		return NULL;
	get_coordinates(sv, &x, &y);

	completion_dialog = gtk_dialog_new_with_buttons(
		"gedit completion",
		NULL,
		GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_NO_SEPARATOR,
		NULL);
	gtk_window_set_decorated(GTK_WINDOW(completion_dialog), FALSE);
	gtk_window_move(GTK_WINDOW(completion_dialog), x, y);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(completion_dialog)->vbox), entry);
	gtk_dialog_set_default_response(GTK_DIALOG(completion_dialog), GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(activate_entry), completion_dialog);
	
	response = gtk_dialog_run(GTK_DIALOG(completion_dialog));
	if (response == GTK_RESPONSE_OK)
		return gtk_entry_get_text(GTK_ENTRY(entry));
	else
		return NULL;
}

/* Main autocomplete function. Insert the selected word in the buffer */

void sourceview_autocomplete(Sourceview* sv)
{
	const gchar* replacement = get_word_replacement(sv);
	const gchar* current_word = ianjuta_editor_get_current_word(IANJUTA_EDITOR(sv), NULL);
	GtkTextBuffer * buffer = GTK_TEXT_BUFFER(sv->priv->document);
	GtkTextIter cursor_iter, *word_iter;
	if (replacement == NULL)
		return;
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, 
								 gtk_text_buffer_get_insert(buffer));
	word_iter = gtk_text_iter_copy(&cursor_iter);
	gtk_text_iter_set_line_index(word_iter, gtk_text_iter_get_line_index(&cursor_iter) - strlen(current_word));
	gtk_text_buffer_delete(buffer, word_iter, &cursor_iter);
	gtk_text_buffer_insert_at_cursor(buffer, replacement, strlen(replacement));
}

 
 
