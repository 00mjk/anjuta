/*
 * dnd.c - These are generic functions which add Drag N' Drop support
 * to an application.
 *
 * Copyright (C) 2000 Jos�� Antonio Caminero Granja
 *
 * Author(s): 
 * 	Jos�� Antonio Caminero Granja <JCamGra@alumnos.uva.es>>
 *  Archit Baweja <bighead@crosswinds.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <gnome.h>

#include "dnd.h"

/*
 * Table (array) of mime types for which "droppings" will be accepted.
 */
static GtkTargetEntry dnd_mime_accepted [DND_MAX_MIME_TYPES];

/*
 * Number of entries in the mime type table for "droppings".
 */
static int dnd_mime_table_length = 0;

/*
 * A pointer to the user supplied function which handles the "droppings".
 */
static void (* dnd_data_dropped) (const gchar *uri, gpointer user_data) = NULL;

/*
 * Callback for the drag_data_received signal, emitted whenever something is
 * dragged onto the widget.
 */
static void
drag_data_received_cb (GtkWidget *widget, GdkDragContext *context,
		       gint x, gint y, GtkSelectionData *data,
		       guint info, guint time, gpointer user_data)
{
	gchar *current, *current_end, *current_fixed;
	gchar *hostname, *filename;
	
	/*
	 * Check to see that we got the name of the file. Impossible that it is
	 * NULL.
	 */	
	g_return_if_fail (data->data != NULL);

	current = data->data;

	while (*current)
	{
		/*
		 * get each file:path in buffer and process
		 */
		current_end = current;
		while (*current_end && *current_end != '\n') current_end++;

		/* remove the \r if necessary and end the string */
		if (*current_end == '\n' && current_end != current &&
				*(current_end - 1) == '\r')
		{
			*(current_end - 1) = '\0';
		}
		else
		{
			*current_end = '\0';
		}
		dnd_data_dropped (current, user_data);
		/*
		filename = g_filename_from_uri (current, &hostname, NULL);
		if (! filename && ! hostname)
		{
			// Some dumb software drops URI without "file:" in the begining
			current_fixed = g_strconcat("file:", current, NULL);
			filename = g_filename_from_uri (current, &hostname, NULL);
			g_free (current_fixed);
		}

		if (filename)
		{
			if (hostname)
			{
				g_warning (_("File %s is not local."), filename);
				g_free (hostname);
			}
			dnd_data_dropped (filename, user_data);
			g_free (filename);
		}
		else
		{
			g_warning (_("Invalid filename %s."), current);
		}
		*/
		current = current_end + 1;
	}
	
	return;
}

/*
 * Initialize widget to start accepting "droppings" for the (NULL terminated)
 * list of mime types.
 */
void
dnd_drop_init (GtkWidget *widget,
	       void (* data_dropped) (const gchar *uri, gpointer user_data),
	       gpointer user_data, ...)
{
	va_list list;
	gchar *mime_type;

	/*
	 * Defensive progamming at display! Check for NULL parameters.
	 */
	g_return_if_fail (widget != NULL);
	g_return_if_fail (data_dropped != NULL);
	g_return_if_fail (dnd_data_dropped == NULL);

	/*
	 * Get all the mime types given by user and prepare the GtkTargetEntry
	 * structure.
	 */
	dnd_mime_table_length = 0;
	va_start (list, user_data);
	while ((mime_type = va_arg (list, gchar *)) != NULL) {
		g_assert (mime_type != NULL);
		g_assert (dnd_mime_table_length < DND_MAX_MIME_TYPES);

		/*
		 * Fill the values.
		 */
		dnd_mime_accepted [dnd_mime_table_length].target = mime_type;
		dnd_mime_accepted [dnd_mime_table_length].flags = 0;
		dnd_mime_accepted [dnd_mime_table_length].info =
			dnd_mime_table_length;
		dnd_mime_table_length++;
	}
	va_end (list);

	/*
	 * Assign the address of the user supplied function (which will handle
	 * the "droppings") to our own global pointer. 
	 */
	dnd_data_dropped = *data_dropped;

	/*
	 * Set the widget to start accepting "droppings" for the given mime
	 * types.
	 */
	gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL, dnd_mime_accepted,
			   dnd_mime_table_length, GDK_ACTION_COPY);

	/*
	 * Connect callback for the "drag_data_received" signal, emitted by the
	 * wigdet whenever a "drop" is made.
	 */
	g_signal_connect (G_OBJECT (widget), "drag_data_received",
					  G_CALLBACK (drag_data_received_cb),
					  (gpointer) user_data);
	return;
}

void
dnd_drop_finalize (GtkWidget *widget, gpointer user_data)
{
	g_signal_handlers_disconnect_by_func (G_OBJECT (widget),
					G_CALLBACK (drag_data_received_cb), user_data);
	dnd_data_dropped = NULL;
}
