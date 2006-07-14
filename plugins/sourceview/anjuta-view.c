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

enum {
	CHAR_ADDED,
	LAST_SIGNAL
};

static guint view_signals[LAST_SIGNAL] = { 0 };

enum
{
	TAG = 0,
	AUTOCOMPLETE,
	SCOPE,
};

enum
{
	ANJUTA_VIEW_POPUP = 1
};

struct _AnjutaViewPrivate
{
	GtkTooltips *tooltips;
	GtkWidget* popup;
	guint scroll_idle;
	
	/* Tag and Autocompletion */
	GList* tag_windows;
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

static gboolean
scroll_to_cursor_real (AnjutaView *view)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_val_if_fail (buffer != NULL, FALSE);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      0.25,
				      FALSE,
				      0.0,
				      0.0);
	
	view->priv->scroll_idle = 0;
	return FALSE;
}

static void
document_read_only_notify_handler (AnjutaDocument *document, 
			           GParamSpec    *pspec,
				   AnjutaView     *view)
{
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !anjuta_document_get_readonly (document));
}

/* Tag stuff */
static TagWindow* get_active_tag_window(AnjutaView* view)
{
	GList* node = view->priv->tag_windows;
	while (node)
	{
		if (tag_window_is_active(TAG_WINDOW(node->data)))
			return TAG_WINDOW(node->data);
		node = g_list_next(node);
	}
	return NULL;
}

static TagWindow* find_tag_window(AnjutaView* view, guint keyval)
{
	GList* node = view->priv->tag_windows;
	while (node)
	{
		if (tag_window_filter_keypress(TAG_WINDOW(node->data), keyval)
			== TAG_WINDOW_KEY_UPDATE)
			return TAG_WINDOW(node->data);
		node = g_list_next(node);
	}
	return NULL;
}

static void
tag_window_selected(GtkWidget* window, gchar* tag_name, AnjutaView* view)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	GtkTextIter cursor_iter, *word_iter;
	gchar* current_word = anjuta_document_get_current_word(ANJUTA_DOCUMENT(buffer));
	
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor_iter, 
								 gtk_text_buffer_get_insert(buffer));
	word_iter = gtk_text_iter_copy(&cursor_iter);
	gtk_text_iter_set_line_index(word_iter, 
		gtk_text_iter_get_line_index(&cursor_iter) - strlen(current_word));
	gtk_text_buffer_delete(buffer, word_iter, &cursor_iter);
	gtk_text_buffer_insert_at_cursor(buffer, tag_name, 
		strlen(tag_name));
		
	g_free(current_word);
	gtk_text_iter_free(word_iter);
}

static void
anjuta_view_set_property (GObject * object,
			   guint property_id,
			   const GValue * value, GParamSpec * pspec)
{
	AnjutaView *self = ANJUTA_VIEW (object);
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		case ANJUTA_VIEW_POPUP:
		{
			GtkWidget* widget;
			self->priv->popup = g_value_get_object (value);
			widget = gtk_menu_get_attach_widget(GTK_MENU(self->priv->popup));
			/* This is a very ugly hack, maybe somebody more familiar with gtk menus
			can fix this */
			if (widget != NULL)
				gtk_menu_detach(GTK_MENU(self->priv->popup));
			gtk_menu_attach_to_widget(GTK_MENU(self->priv->popup), GTK_WIDGET(self), NULL);
			break;
		}
		default:
		{
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
		}
	}
}

static void
anjuta_view_get_property (GObject * object,
			   guint property_id,
			   GValue * value, GParamSpec * pspec)
{
	AnjutaView *self = ANJUTA_VIEW (object);
	
	g_return_if_fail(value != NULL);
	g_return_if_fail(pspec != NULL);
	
	switch (property_id)
	{
		case ANJUTA_VIEW_POPUP:
		{
			g_value_set_object (value, self->priv->popup);
			break;
		}
		default:
		{
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
			break;
		}
	}
}

static void
anjuta_view_class_init (AnjutaViewClass *klass)
{
	GObjectClass     *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass   *gtkobject_class = GTK_OBJECT_CLASS (klass);
	GtkTextViewClass *textview_class = GTK_TEXT_VIEW_CLASS (klass);
	GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);
	GtkBindingSet    *binding_set;
	GParamSpec *anjuta_view_spec_popup;

	gtkobject_class->destroy = anjuta_view_destroy;
	object_class->finalize = anjuta_view_finalize;
	object_class->set_property = anjuta_view_set_property;
	object_class->get_property = anjuta_view_get_property;	

	widget_class->focus_out_event = anjuta_view_focus_out;
	widget_class->expose_event = anjuta_view_expose;
	widget_class->key_press_event = anjuta_view_key_press_event;
	widget_class->button_press_event = anjuta_view_button_press_event;

	textview_class->move_cursor = anjuta_view_move_cursor;

	g_type_class_add_private (klass, sizeof (AnjutaViewPrivate));
	
	anjuta_view_spec_popup = g_param_spec_object ("popup",
						       "Popup menu",
						       "The popup-menu to show",
						       GTK_TYPE_WIDGET,
						       G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 ANJUTA_VIEW_POPUP,
					 anjuta_view_spec_popup);
	
		view_signals[CHAR_ADDED] =
   		g_signal_new ("char_added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AnjutaViewClass, char_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__CHAR,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_CHAR);
	
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
	
	view->priv->tag_windows = NULL;
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

	if (view->priv->popup != NULL)
	    gtk_menu_detach (GTK_MENU (view->priv->popup));

	g_list_free(view->priv->tag_windows);
		
	(* G_OBJECT_CLASS (anjuta_view_parent_class)->finalize) (object);
}

static gint
anjuta_view_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	AnjutaView *view = ANJUTA_VIEW (widget);
	TagWindow* tag_window = get_active_tag_window(view);
	if (tag_window != NULL)
	{
		gtk_widget_hide(GTK_WIDGET(tag_window));
	}
	
	gtk_widget_queue_draw (widget);
	
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
	g_return_if_fail (ANJUTA_IS_VIEW (view));
	
	view->priv->scroll_idle = g_idle_add ((GSourceFunc) scroll_to_cursor_real, view);
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

void anjuta_view_register_completion(AnjutaView* view, TagWindow* tagwin)
{
	g_return_if_fail(tagwin != NULL);
	
	view->priv->tag_windows = g_list_append(view->priv->tag_windows, tagwin);
	g_signal_connect(G_OBJECT(tagwin), "selected", G_CALLBACK(tag_window_selected), view);
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

static gchar
anjuta_view_get_last_character(GtkWidget* widget)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	GtkTextIter cursor;
	
	gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
	return (gchar) gtk_text_iter_get_char(&cursor);
}

static gboolean
anjuta_view_key_press_event		(GtkWidget *widget, GdkEventKey       *event)
{
	AnjutaView* view = ANJUTA_VIEW(widget);
	TagWindow* tag_window = get_active_tag_window(view);
	
	 switch (event->keyval)
	 {
		case GDK_Shift_L:
		case GDK_Shift_R:
		{
			return TRUE;
		}
		default:
		{
			gboolean retval;
			if (tag_window != NULL)
			{
				switch (tag_window_filter_keypress(tag_window, event->keyval))
				{
					case TAG_WINDOW_KEY_UPDATE:
					{
						retval = (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
						tag_window_update(tag_window, GTK_WIDGET(view));
						g_signal_emit_by_name(G_OBJECT(view), "char_added", anjuta_view_get_last_character(widget));
						return retval;
					}
					case TAG_WINDOW_KEY_CONTROL:
					{
						return TRUE;
					}
					case TAG_WINDOW_KEY_SKIP:
					{
						gtk_widget_hide(GTK_WIDGET(tag_window));
					}
				}
			}
			if ((tag_window = find_tag_window(view, event->keyval)) != NULL)
			{
				retval = (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
				tag_window_update(tag_window, GTK_WIDGET(view));
				g_signal_emit_by_name(G_OBJECT(view), "char_added", anjuta_view_get_last_character(widget));
				return retval;
			}
			retval = (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->key_press_event)(widget, event);
			g_signal_emit_by_name(G_OBJECT(view), "char_added", anjuta_view_get_last_character(widget));
			return retval;
		}				
	}
}

static gboolean	anjuta_view_button_press_event		(GtkWidget         *widget, GdkEventButton       *event)
{
	AnjutaView* view = ANJUTA_VIEW(widget);
	TagWindow* tag_window = get_active_tag_window(view);
	if (tag_window != NULL)
	{
		gtk_widget_hide(GTK_WIDGET(tag_window));
	}
	
	switch(event->button)
	{
		case 3: /* Right Button */
		{
			gtk_menu_popup (GTK_MENU (view->priv->popup), NULL, NULL, NULL, NULL, 
                  event->button, event->time);
			return TRUE;
		}
		default:
			return (* GTK_WIDGET_CLASS (anjuta_view_parent_class)->button_press_event)(widget, event);
	}
}
