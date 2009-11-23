/***************************************************************************
 *            sourceview-cell.c
 *
 *  So Mai 21 14:44:13 2006
 *  Copyright  2006  Johannes Schmid
 *  jhs@cvs.gnome.org
 ***************************************************************************/

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

#include "sourceview-cell.h"

#include <libanjuta/interfaces/ianjuta-editor-cell.h>
#include <libanjuta/interfaces/ianjuta-editor-cell-style.h>
#include <libanjuta/interfaces/ianjuta-iterable.h>
#include <libanjuta/anjuta-utils.h>
#include <libanjuta/anjuta-debug.h>

#include <gtk/gtk.h>
#include <string.h>

static void sourceview_cell_class_init(SourceviewCellClass *klass);
static void sourceview_cell_instance_init(SourceviewCell *sp);
static void sourceview_cell_finalize(GObject *object);

struct _SourceviewCellPrivate {
	GtkTextIter iter;
	GtkTextView* view;
	GtkTextBuffer* buffer;
};

static gpointer sourceview_cell_parent_class = NULL;

static void
sourceview_cell_class_init(SourceviewCellClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	sourceview_cell_parent_class = g_type_class_peek_parent(klass);
	
	
	object_class->finalize = sourceview_cell_finalize;
}

static void
sourceview_cell_instance_init(SourceviewCell *obj)
{
	obj->priv = g_slice_new(SourceviewCellPrivate);
	
	/* Initialize private members, etc. */	
}

static void
sourceview_cell_finalize(GObject *object)
{
	SourceviewCell *cobj;
	cobj = SOURCEVIEW_CELL(object);
	
	g_slice_free(SourceviewCellPrivate, cobj->priv);
	G_OBJECT_CLASS(sourceview_cell_parent_class)->finalize(object);
}

SourceviewCell *
sourceview_cell_new(GtkTextIter* iter, GtkTextView* view)
{
	SourceviewCell *obj;
	
	obj = SOURCEVIEW_CELL(g_object_new(SOURCEVIEW_TYPE_CELL, NULL));
	
	obj->priv->buffer = gtk_text_view_get_buffer(view);
	obj->priv->iter = *iter;
	obj->priv->view = view;
	
	return obj;
}

GtkTextIter*
sourceview_cell_get_iter (SourceviewCell* cell)
{
	return &cell->priv->iter;
}

static gchar*
icell_get_character(IAnjutaEditorCell* icell, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(icell);
	gunichar c = gtk_text_iter_get_char (&cell->priv->iter);
	gchar* outbuf = g_new0(gchar, 6);
	g_unichar_to_utf8 (c, outbuf);
	return outbuf;
}

static gint 
icell_get_length(IAnjutaEditorCell* icell, GError** e)
{
	gchar* text = icell_get_character(icell, e);
	gint retval = 0;
	if (text)
		retval = g_utf8_strlen (text, -1);
	g_free(text);
	return retval;
}

static gchar
icell_get_char(IAnjutaEditorCell* icell, gint index, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(icell);
	gunichar c = gtk_text_iter_get_char (&cell->priv->iter);
	gchar* outbuf = g_new0(gchar, 6);
	gint len = g_unichar_to_utf8 (c, outbuf);
	gchar retval;
	if (index < len)
		retval = outbuf[index];
	else
		retval = 0;
	g_free (outbuf);
	return retval;
}

static IAnjutaEditorAttribute
icell_get_attribute (IAnjutaEditorCell* icell, GError **e)
{
	IAnjutaEditorAttribute attrib = IANJUTA_EDITOR_TEXT;
	return attrib;
}

static void
icell_iface_init(IAnjutaEditorCellIface* iface)
{
	iface->get_character = icell_get_character;
	iface->get_char = icell_get_char;
	iface->get_length = icell_get_length;
	iface->get_attribute = icell_get_attribute;
}

static
GtkTextAttributes* get_attributes(GtkTextIter* iter, GtkTextView* view)
{
	GtkTextAttributes* atts = gtk_text_view_get_default_attributes(view);
	gtk_text_iter_get_attributes(iter, atts);
	return atts;
}

static gchar*
icell_style_get_font_description(IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	const gchar* font;
	SourceviewCell* cell = SOURCEVIEW_CELL(icell_style);
	GtkTextAttributes* atts = get_attributes(&cell->priv->iter, 
																					 cell->priv->view);
	font = pango_font_description_to_string(atts->font);
	g_free(atts);
	return g_strdup(font);
}

static gchar*
icell_style_get_color(IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	gchar* color;
	SourceviewCell* cell = SOURCEVIEW_CELL(icell_style);
	GtkTextAttributes* atts = get_attributes(&cell->priv->iter, cell->priv->view);
	color = anjuta_util_string_from_color(atts->appearance.fg_color.red,
																				atts->appearance.fg_color.green, atts->appearance.fg_color.blue);
	g_free(atts);
	return color;
}

static gchar*
icell_style_get_background_color(IAnjutaEditorCellStyle* icell_style, GError ** e)
{
	gchar* color;
	SourceviewCell* cell = SOURCEVIEW_CELL(icell_style);
	GtkTextAttributes* atts = get_attributes(&cell->priv->iter, cell->priv->view);
	color = anjuta_util_string_from_color(atts->appearance.bg_color.red,
																				atts->appearance.bg_color.green, atts->appearance.bg_color.blue);
	g_free(atts);
	return color;
}

static void
icell_style_iface_init(IAnjutaEditorCellStyleIface* iface)
{
	iface->get_font_description = icell_style_get_font_description;
	iface->get_color = icell_style_get_color;
	iface->get_background_color = icell_style_get_background_color;
}

static gboolean
iiter_first(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	gboolean retval = gtk_text_iter_is_start (&cell->priv->iter);
	if (!retval)
		gtk_text_iter_set_offset (&cell->priv->iter, 0);
	return retval;
}

static gboolean
iiter_next(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	
	return gtk_text_iter_forward_char(&cell->priv->iter);
}

static gboolean
iiter_previous(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	
	return gtk_text_iter_backward_char(&cell->priv->iter);
}

static gboolean
iiter_last(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	gboolean retval = gtk_text_iter_is_end (&cell->priv->iter);
	if (retval)
		gtk_text_iter_forward_to_end(&cell->priv->iter);
	return retval;
}

static void
iiter_foreach(IAnjutaIterable* iter, GFunc callback, gpointer data, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	
	gint saved_offset;
	
	saved_offset = gtk_text_iter_get_offset (&cell->priv->iter);
	gtk_text_iter_set_offset(&cell->priv->iter, 0);
	while (gtk_text_iter_forward_char(&cell->priv->iter))
	{
		(*callback)(cell, data);
	}
	gtk_text_iter_set_offset(&cell->priv->iter, saved_offset);
}

static gboolean
iiter_set_position (IAnjutaIterable* iter, gint position, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	
	gtk_text_iter_set_offset (&cell->priv->iter, position);
	return TRUE;
}

static gint
iiter_get_position(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	return gtk_text_iter_get_offset(&cell->priv->iter);
}

static gint
iiter_get_length(IAnjutaIterable* iter, GError** e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	
	return gtk_text_buffer_get_char_count (gtk_text_iter_get_buffer(&cell->priv->iter));
}

static IAnjutaIterable *
iiter_clone (IAnjutaIterable *iter, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	
	return IANJUTA_ITERABLE (sourceview_cell_new (&cell->priv->iter, cell->priv->view));
}

static void
iiter_assign (IAnjutaIterable *iter, IAnjutaIterable *src_iter, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	SourceviewCell* src_cell = SOURCEVIEW_CELL(src_iter);
	
	cell->priv->iter = src_cell->priv->iter;
}

static gint
iiter_compare (IAnjutaIterable *iter, IAnjutaIterable *other_iter, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	SourceviewCell* other_cell = SOURCEVIEW_CELL(other_iter);
	
	return gtk_text_iter_compare (&cell->priv->iter, &other_cell->priv->iter);
}

static gint
iiter_diff (IAnjutaIterable *iter, IAnjutaIterable *other_iter, GError **e)
{
	SourceviewCell* cell = SOURCEVIEW_CELL(iter);
	SourceviewCell* other_cell = SOURCEVIEW_CELL(other_iter);
	
	return (gtk_text_iter_get_offset (&other_cell->priv->iter) 
					- gtk_text_iter_get_offset (&cell->priv->iter));
}

static void
iiter_iface_init(IAnjutaIterableIface* iface)
{
	iface->first = iiter_first;
	iface->next = iiter_next;
	iface->previous = iiter_previous;
	iface->last = iiter_last;
	iface->foreach = iiter_foreach;
	iface->set_position = iiter_set_position;
	iface->get_position = iiter_get_position;
	iface->get_length = iiter_get_length;
	iface->assign = iiter_assign;
	iface->clone = iiter_clone;
	iface->diff = iiter_diff;
	iface->compare = iiter_compare;
}


ANJUTA_TYPE_BEGIN(SourceviewCell, sourceview_cell, G_TYPE_OBJECT);
ANJUTA_TYPE_ADD_INTERFACE(icell, IANJUTA_TYPE_EDITOR_CELL);
ANJUTA_TYPE_ADD_INTERFACE(icell_style, IANJUTA_TYPE_EDITOR_CELL_STYLE);
ANJUTA_TYPE_ADD_INTERFACE(iiter, IANJUTA_TYPE_ITERABLE);
ANJUTA_TYPE_END;
