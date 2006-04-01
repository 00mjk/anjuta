/*
 * anjuta-view.c
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2003-2005 Paolo Maggi
 * Copyright (C) 2006 Johannes Schmid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the anjuta Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the anjuta Team.  
 * See the ChangeLog files for a list of changes.
 *
 * $Id$ 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>

#include <libanjuta/anjuta-debug.h>

#include "anjuta-view.h"
#include "anjuta-encodings.h"
#include "sourceview.h"
#include "sourceview-private.h"
#include "tag-window.h"

#define ANJUTA_VIEW_SCROLL_MARGIN 0.02

#define ANJUTA_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ANJUTA_TYPE_VIEW, AnjutaViewPrivate))

typedef struct _AnjutaTags AnjutaTags;

enum
{
	TAG = 0,
	AUTOCOMPLETE,
	SCOPE,
};

struct _AnjutaTags
{
	gchar* current_word;
	TagWindow* tag_window;
	GtkListStore* model;
	gint type;
	
	/* tag fill */
	gboolean (*update_tags)(AnjutaView* view, GtkListStore* store, gchar* current_word);
	
	/* scope fill */
	gboolean (*update_scope)(AnjutaView* view, GtkListStore* store, gchar* current_word);
	
	/* autocomplete fill */
	gboolean (*update_autocomplete)(AnjutaView* view, GtkListStore* store, gchar* current_word);
};

struct _AnjutaViewPrivate
{
	GtkTooltips *tooltips;

	/* idle hack to make open-at-line work */
	guint        scroll_idle;
	
	/* Tag and Autocompletion */
	AnjutaTags* tags;
};

static void	anjuta_view_destroy		(GtkObject       *object);
static void	anjuta_view_finalize		(GObject         *object);
static void	anjuta_view_move_cursor		(GtkTextView     *text_view,
						 GtkMovementStep  step,
						 gint             count,
						 gboolean         extend_selection);
static gint     anjuta_view_focus_out		(GtkWidget       *widget,
						 GdkEventFocus   *event);

static gint	anjuta_view_expose	 	(GtkWidget       *widget,
						 GdkEventExpose  *event);
					
static gboolean	anjuta_view_key_press_event		(GtkWidget         *widget,
							 GdkEventKey       *event);
static gboolean	anjuta_view_button_press_event		(GtkWidget         *widget,
							 GdkEventButton       *event);

G_DEFINE_TYPE(AnjutaView, anjuta_view, GTK_TYPE_SOURCE_VIEW)


static void
document_read_only_notify_handler (AnjutaDocument *document, 
			           GParamSpec    *pspec,
				   AnjutaView     *view)
{
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !anjuta_document_get_readonly (document));
}

/* Tag stuff */
static void
tag_window_selected(GtkWidget* window, gchar* tag_name, AnjutaView* view)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GtkTextIter cursor_iter, *word_iter;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, 
								 gtk_text_buffer_get_insert(buffer));
	word_iter = gtk_text_iter_copy(&cursor_iter);
	gtk_text_iter_set_line_index(word_iter, 
		gtk_text_iter_get_line_index(&cursor_iter) - strlen(view->priv->tags->current_word));
	gtk_text_buffer_delete(buffer, word_iter, &cursor_iter);
	gtk_text_buffer_insert_at_cursor(buffer, tag_name, 
		strlen(tag_name));
		
	g_free(view->priv->tags->current_word);
	view->priv->tags->current_word = NULL;
	gtk_text_iter_free(word_iter);
}

static void tag_window_hidden(GtkWidget* widget, AnjutaView* view)
{
	gtk_list_store_clear(tag_window_get_model(TAG_WINDOW(widget)));
	view->priv->tags->type = TAG;
}

/* Return a tuple containing the (x, y) position of the cursor + 1 line */
static void
get_coordinates(AnjutaView* view, int* x, int* y)
{
	int xor, yor;
	/* We need to Rectangles because if we step to the next line
	the x position is lost */
	GdkRectangle rectx;
	GdkRectangle recty;
	GdkWindow* window;
	GtkTextIter cursor;
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer)); 
	gtk_text_iter_backward_chars(&cursor, strlen(view->priv->tags->current_word));
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &cursor, &rectx);
	gtk_text_iter_forward_lines(&cursor, 1);
	gtk_text_view_get_iter_location(GTK_TEXT_VIEW(view), &cursor, &recty);
	window = gtk_text_view_get_window(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT);
	gtk_text_view_buffer_to_window_coords(GTK_TEXT_VIEW(view), GTK_TEXT_WINDOW_TEXT, 
		rectx.x + rectx.width, recty.y, x, y);
	
	gdk_window_get_origin(window, &xor, &yor);
	*x = *x + xor;
	*y = *y + yor;
}

static void
anjuta_view_update_tags(AnjutaView* view)
{
	AnjutaDocument* doc = ANJUTA_DOCUMENT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
	gint x, y;
	
	g_return_if_fail(view->priv->tags->tag_window != NULL);
	
	if (view->priv->tags->update_tags == NULL)
		return;
	
	g_free(view->priv->tags->current_word);
	view->priv->tags->current_word = anjuta_document_get_current_word(doc);
	
	if (view->priv->tags->current_word == NULL)
		return;
	
	if (view->priv->tags->update_tags
		(view, tag_window_get_model(view->priv->tags->tag_window), view->priv->tags->current_word))
	{
		view->priv->tags->type = TAG;
		get_coordinates(view, &x, &y);	
		gtk_window_move(GTK_WINDOW(view->priv->tags->tag_window), x, y);
		gtk_widget_show(GTK_WIDGET(view->priv->tags->tag_window));
	}
}

void
anjuta_view_set_tag_update(AnjutaView* view, AnjutaViewUpdateFunc update)
{
	view->priv->tags->update_tags = update;
}

void
anjuta_view_set_autocomplete_update(AnjutaView* view, AnjutaViewUpdateFunc update)
{
	view->priv->tags->update_autocomplete = update;
}

void
anjuta_view_autocomplete(AnjutaView* view)
{
	AnjutaDocument* doc = ANJUTA_DOCUMENT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
	if (view->priv->tags->update_autocomplete == NULL)
		return;
	
	g_free(view->priv->tags->current_word);
	view->priv->tags->current_word = anjuta_document_get_current_word(doc);
	
	if (view->priv->tags->update_autocomplete
		(view, tag_window_get_model(view->priv->tags->tag_window), view->priv->tags->current_word))
	{
		int x,y;
		view->priv->tags->type = AUTOCOMPLETE;
		get_coordinates(view, &x, &y);	
		gtk_window_move(GTK_WINDOW(view->priv->tags->tag_window), x, y);
		gtk_widget_show(GTK_WIDGET(view->priv->tags->tag_window));
	}
	else
		view->priv->tags->type = TAG;
}

static void
anjuta_view_class_init (AnjutaViewClass *klass)
{
	GObjectClass     *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass   *gtkobject_class = GTK_OBJECT_CLASS (klass);
	GtkTextViewClass *textview_class = GTK_TEXT_VIEW_CLASS (klass);
	GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);
	GtkBindingSet    *binding_set;

	gtkobject_class->destroy = anjuta_view_destroy;
	object_class->finalize = anjuta_view_finalize;

	widget_class->focus_out_event = anjuta_view_focus_out;
	widget_class->expose_event = anjuta_view_expose;
	widget_class->key_press_event = anjuta_view_key_press_event;
	widget_class->button_press_event = anjuta_view_button_press_event;

	textview_class->move_cursor = anjuta_view_move_cursor;

	g_type_class_add_private (klass, sizeof (AnjutaViewPrivate));
	
	binding_set = gtk_binding_set_by_class (klass);	
}

static void
move_cursor (GtkTextView       *text_view,
	     const GtkTextIter *new_location,
	     gboolean           extend_selection)
{
	GtkTextBuffer *buffer = text_view->buffer;

	if (extend_selection)
		gtk_text_buffer_move_mark_by_name (buffer,
						   "insert",
						   new_location);
	else
		gtk_text_buffer_place_cursor (buffer, new_location);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (text_view),
				      gtk_text_buffer_get_insert (buffer),
				      ANJUTA_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

static void
anjuta_view_move_cursor (GtkTextView    *text_view,
			GtkMovementStep step,
			gint            count,
			gboolean        extend_selection)
{
	GtkTextBuffer *buffer = text_view->buffer;
	GtkTextMark *mark;
	GtkTextIter cur, iter;

	/* really make sure gtksourceview's home/end is disabled */
	g_return_if_fail (!gtk_source_view_get_smart_home_end (
						GTK_SOURCE_VIEW (text_view)));

	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
	iter = cur;

	if (step == GTK_MOVEMENT_DISPLAY_LINE_ENDS &&
	    (count == -1) && gtk_text_iter_starts_line (&iter))
	{
		/* Find the iter of the first character on the line. */
		while (!gtk_text_iter_ends_line (&cur))
		{
			gunichar c;

			c = gtk_text_iter_get_char (&cur);
			if (g_unichar_isspace (c))
				gtk_text_iter_forward_char (&cur);
			else
				break;
		}

		/* if we are clearing selection, we need to move_cursor even
		 * if we are at proper iter because selection_bound may need
		 * to be moved */
		if (!gtk_text_iter_equal (&cur, &iter) || !extend_selection)
			move_cursor (text_view, &cur, extend_selection);
	}
	else if (step == GTK_MOVEMENT_DISPLAY_LINE_ENDS &&
	         (count == 1) && gtk_text_iter_ends_line (&iter))
	{
		/* Find the iter of the last character on the line. */
		while (!gtk_text_iter_starts_line (&cur))
		{
			gunichar c;

			gtk_text_iter_backward_char (&cur);
			c = gtk_text_iter_get_char (&cur);
			if (!g_unichar_isspace (c))
			{
				/* We've gone one character too far. */
				gtk_text_iter_forward_char (&cur);
				break;
			}
		}
		
		/* if we are clearing selection, we need to move_cursor even
		 * if we are at proper iter because selection_bound may need
		 * to be moved */
		if (!gtk_text_iter_equal (&cur, &iter) || !extend_selection)
			move_cursor (text_view, &cur, extend_selection);
	}
	else
	{
		/* note that we chain up to GtkTextView skipping GtkSourceView */
		(* GTK_TEXT_VIEW_CLASS (anjuta_view_parent_class)->move_cursor) (text_view,
										step,
										count,
										extend_selection);
	}
}

static gboolean
scroll_to_cursor_on_init (AnjutaView *view)
{
	anjuta_view_scroll_to_cursor (view);

	view->priv->scroll_idle = 0;
	return FALSE;
}

static void 
anjuta_view_init (AnjutaView *view)
{	
	view->priv = ANJUTA_VIEW_GET_PRIVATE (view);

	/*
	 *  Set tab, fonts, wrap mode, colors, etc. according
	 *  to preferences 
	 */
		
	g_object_set (G_OBJECT (view), 
		      "wrap_mode", FALSE,
		      "show_line_numbers", TRUE,
		      "auto_indent", TRUE,
		      "tabs_width", 4,
		      "insert_spaces_instead_of_tabs", FALSE,
		      "highlight_current_line", TRUE, 
		      "smart_home_end", FALSE, /* Never changes this */
		      NULL);

	/* Make sure that the view is scrolled to the cursor so
	 * that "anjuta +100 foo.txt" works.
	 * We would like to this on the first expose handler so that
	 * it would look instantaneous, but this isn't currently
	 * possible: see bug #172277 and bug #311728.
	 * So we need to do this in an idle handler.
	 */
	view->priv->scroll_idle = g_idle_add ((GSourceFunc) scroll_to_cursor_on_init, view); 
	
	view->priv->tags = g_new0(AnjutaTags, 1);
	view->priv->tags->tag_window = TAG_WINDOW(tag_window_new());
	g_signal_connect(G_OBJECT(view->priv->tags->tag_window), 
		"selected", G_CALLBACK(tag_window_selected), view);
	g_signal_connect(G_OBJECT(view->priv->tags->tag_window), 
		"hidden", G_CALLBACK(tag_window_hidden), view);
}

static void
anjuta_view_destroy (GtkObject *object)
{
	AnjutaView *view;

	view = ANJUTA_VIEW (object);
	
	(* GTK_OBJECT_CLASS (anjuta_view_parent_class)->destroy) (object);
}

static void
anjuta_view_finalize (GObject *object)
{
	AnjutaView *view;

	view = ANJUTA_VIEW (object);
	
	if (view->priv->tooltips != NULL)
		g_object_unref (view->priv->tooltips);

	if (view->priv->scroll_idle > 0)
		g_source_remove (view->priv->scroll_idle);
	
	g_free(view->priv->tags->current_word);
	g_free(view->priv->tags);
		
	(* G_OBJECT_CLASS (anjuta_view_parent_class)->finalize) (object);
}

static gint
anjuta_view_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	AnjutaView *view = ANJUTA_VIEW (widget);
	
	gtk_widget_queue_draw (widget);
	gtk_widget_hide(GTK_WIDGET(view->priv->tags->tag_window));
	
	(* GTK_WIDGET_CLASS (anjuta_view_parent_class)->focus_out_event) (widget, event);
	
	return FALSE;
}


/**
 * anjuta_view_new:
 * @doc: a #AnjutaDocument
 * 
 * Creates a new #AnjutaView object displaying the @doc document. 
 * @doc cannot be NULL.
 *
 * Return value: a new #AnjutaView
 **/
GtkWidget *
anjuta_view_new (AnjutaDocument *doc)
{
	GtkWidget *view;

	g_return_val_if_fail (ANJUTA_IS_DOCUMENT (doc), NULL);

	view = GTK_WIDGET (g_object_new (ANJUTA_TYPE_VIEW, NULL));
	
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (view),
				  GTK_TEXT_BUFFER (doc));
  		
	g_signal_connect (doc,
			  "notify::read-only",
			  G_CALLBACK (document_read_only_notify_handler),
			  view);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !anjuta_document_get_readonly (doc));					  

	gtk_widget_show_all (view);

	return view;
}

void
anjuta_view_cut_clipboard (AnjutaView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_cut_clipboard (buffer,
  				       clipboard,
				       !anjuta_document_get_readonly (
				       		ANJUTA_DOCUMENT (buffer)));
  	
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      ANJUTA_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
anjuta_view_copy_clipboard (AnjutaView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

  	gtk_text_buffer_copy_clipboard (buffer, clipboard);

	/* on copy do not scroll, we are already on screen */
}

void
anjuta_view_paste_clipboard (AnjutaView *view)
{
  	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_paste_clipboard (buffer,
					 clipboard,
					 NULL,
					 !anjuta_document_get_readonly (
						ANJUTA_DOCUMENT (buffer)));

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      ANJUTA_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
anjuta_view_delete_selection (AnjutaView *view)
{
  	GtkTextBuffer *buffer = NULL;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_delete_selection (buffer,
					  TRUE,
					  !anjuta_document_get_readonly (
						ANJUTA_DOCUMENT (buffer)));
						
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      ANJUTA_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
anjuta_view_select_all (AnjutaView *view)
{
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_select_range (buffer, &start, &end);
}

void
anjuta_view_scroll_to_cursor (AnjutaView *view)
{
	GtkTextBuffer* buffer = NULL;

	g_return_if_fail (ANJUTA_IS_VIEW (view));
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      0.25,
				      FALSE,
				      0.0,
				      0.0);
}

/* assign a unique name */
static G_CONST_RETURN gchar *
get_widget_name (GtkWidget *w)
{
	const gchar *name;	

	name = gtk_widget_get_name (w);
	g_return_val_if_fail (name != NULL, NULL);

	if (strcmp (name, g_type_name (GTK_WIDGET_TYPE (w))) == 0)
	{
		static guint d = 0;
		gchar *n;

		n = g_strdup_printf ("%s_%u_%u", name, d, (guint) g_random_int);
		d++;

		gtk_widget_set_name (w, n);
		g_free (n);

		name = gtk_widget_get_name (w);
	}

	return name;
}

/* There is no clean way to set the cursor-color, so we are stuck
 * with the following hack: set the name of each widget and parse
 * a gtkrc string.
 */
static void 
modify_cursor_color (GtkWidget *textview, 
		     GdkColor  *color)
{
	static const char cursor_color_rc[] =
		"style \"svs-cc\"\n"
		"{\n"
			"GtkSourceView::cursor-color=\"#%04x%04x%04x\"\n"
		"}\n"
		"widget \"*.%s\" style : application \"svs-cc\"\n";

	const gchar *name;
	gchar *rc_temp;

	name = get_widget_name (textview);
	g_return_if_fail (name != NULL);

	if (color != NULL)
	{
		rc_temp = g_strdup_printf (cursor_color_rc,
					   color->red, 
					   color->green, 
					   color->blue,
					   name);
	}
	else
	{
		GtkRcStyle *rc_style;

 		rc_style = gtk_widget_get_modifier_style (textview);

		rc_temp = g_strdup_printf (cursor_color_rc,
					   rc_style->text [GTK_STATE_NORMAL].red,
					   rc_style->text [GTK_STATE_NORMAL].green,
					   rc_style->text [GTK_STATE_NORMAL].blue,
					   name);
	}

	gtk_rc_parse_string (rc_temp);
	gtk_widget_reset_rc_styles (textview);

	g_free (rc_temp);
}

void
anjuta_view_set_colors (AnjutaView *view,
		       gboolean   def,
		       GdkColor  *backgroud,
		       GdkColor  *text,
		       GdkColor  *selection,
		       GdkColor  *sel_text)
{

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	/* just a bit of paranoia */
	gtk_widget_ensure_style (GTK_WIDGET (view));

	if (!def)
	{
		if (backgroud != NULL)
			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_NORMAL, backgroud);

		if (selection != NULL)
		{
			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_SELECTED, selection);
			gtk_widget_modify_base (GTK_WIDGET (view), 
						GTK_STATE_ACTIVE, selection);
		}

		if (sel_text != NULL)
		{
			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_SELECTED, sel_text);		
			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_ACTIVE, sel_text);		
		}

		if (text != NULL)
		{
			gtk_widget_modify_text (GTK_WIDGET (view), 
						GTK_STATE_NORMAL, text);
			modify_cursor_color (GTK_WIDGET (view), text);
		}
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view));

		rc_style->color_flags [GTK_STATE_NORMAL] = 0;
		rc_style->color_flags [GTK_STATE_SELECTED] = 0;
		rc_style->color_flags [GTK_STATE_ACTIVE] = 0;

		gtk_widget_modify_style (GTK_WIDGET (view), rc_style);

		/* It must be called after the text color has been modified */
		modify_cursor_color (GTK_WIDGET (view), NULL);
	}
}

void
anjuta_view_set_font (AnjutaView   *view, 
		     gboolean     def, 
		     const gchar *font_name)
{

	g_return_if_fail (ANJUTA_IS_VIEW (view));

	if (!def)
	{
		PangoFontDescription *font_desc = NULL;

		g_return_if_fail (font_name != NULL);
		
		font_desc = pango_font_description_from_string (font_name);
		g_return_if_fail (font_desc != NULL);

		gtk_widget_modify_font (GTK_WIDGET (view), font_desc);
		
		pango_font_description_free (font_desc);		
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view));

		if (rc_style->font_desc)
			pango_font_description_free (rc_style->font_desc);

		rc_style->font_desc = NULL;
		
		gtk_widget_modify_style (GTK_WIDGET (view), rc_style);
	}
}

static gint
anjuta_view_expose (GtkWidget      *widget,
                   GdkEventExpose *event)
{
	GtkTextView *text_view;
	AnjutaDocument *doc;
	
	text_view = GTK_TEXT_VIEW (widget);
	
	doc = ANJUTA_DOCUMENT (gtk_text_view_get_buffer (text_view));
	
	if ((event->window == gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT)))
	{
		GdkRectangle visible_rect;
		GtkTextIter iter1, iter2;
		
		gtk_text_view_get_visible_rect (text_view, &visible_rect);
		gtk_text_view_get_line_at_y (text_view, &iter1,
					     visible_rect.y, NULL);
		gtk_text_view_get_line_at_y (text_view, &iter2,
					     visible_rect.y
					     + visible_rect.height, NULL);
		gtk_text_iter_forward_line (&iter2);
	}

	return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->expose_event)(widget, event);
}

static gboolean
anjuta_view_key_press_event		(GtkWidget *widget, GdkEventKey       *event)
{
	AnjutaView* view = ANJUTA_VIEW(widget);
	
	/* Assume keyval is something like [A-Za-z0-9_]+ */
	if  ((GDK_A <= event->keyval && event->keyval <= GDK_Z)
		|| (GDK_a <= event->keyval && event->keyval <= GDK_z)
		|| (GDK_0 <= event->keyval && event->keyval <= GDK_9)		
		|| GDK_underscore == event->keyval
		|| GDK_Shift_L == event->keyval
		|| GDK_Shift_R == event->keyval)
	{	
		/* Add the character to the buffer */
		gboolean retval = (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
		/* Call tag completion */
		switch(view->priv->tags->type)
		{
			case TAG:
				anjuta_view_update_tags(view);
				break;
			case SCOPE:
				break;
			case AUTOCOMPLETE:
				anjuta_view_autocomplete(view);
				break;
		}
		return retval;
	}
	else if (event->keyval == GDK_Down)
	{
		if (tag_window_down(view->priv->tags->tag_window))
			return TRUE;
		else
			return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
	}
	else if (event->keyval == GDK_Up)
	{
		if (tag_window_up(view->priv->tags->tag_window))
			return TRUE;
		else
			return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
	}
	else if (event->keyval == GDK_Return || event->keyval == GDK_Tab)
	{
		if (tag_window_select(view->priv->tags->tag_window))
			return TRUE;
		else
			return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(view->priv->tags->tag_window));
		return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
	}
}

static gboolean	anjuta_view_button_press_event		(GtkWidget         *widget, GdkEventButton       *event)
{
	AnjutaView* view = ANJUTA_VIEW(widget);
	gtk_widget_hide(GTK_WIDGET(view->priv->tags->tag_window));
	return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->button_press_event)(widget, event);
}
