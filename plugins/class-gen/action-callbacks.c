/*
 *  Copyright (C) 2002  Dave Huseby
 *  Copyright (C) 2005 Massimo Cora' [porting to Anjuta 2.x plugin style]
 *
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <libanjuta/anjuta-plugin.h>
#include <libanjuta/anjuta-debug.h>

#include "action-callbacks.h"
#include "class_gen.h"
#include "plugin.h"


void on_cc_button_browse_header_clicked (GtkButton *button, GladeXML* gxml) {
	
	GtkWidget *header_file_widget;
	GtkWidget *source_file_widget;
	const gchar *header_file, *source_file;
	
	/* get the header displaying a browsing window */
	header_file = browse_for_file(_("Select header file"));
	
	if ( header_file == NULL )
		return;

	header_file_widget = glade_xml_get_widget (gxml, "cc_header_file");
	source_file_widget = glade_xml_get_widget (gxml, "cc_source_file");
	
	source_file = gtk_entry_get_text (GTK_ENTRY (source_file_widget));
	
	/* let's set the header in the entry */
	gtk_entry_set_text(GTK_ENTRY (header_file_widget), header_file);
	
	/* is source_file is void then fill it with a "header_file".c */
	if( strlen (source_file) == 0) {
		gchar *s = g_strdup(header_file), *t, *p = strrchr(s, '.');
		if (p == NULL) {
			g_free (s);
			return;
		}
		
		s[strlen(s) - strlen(p)] = '\0';
		t = g_strdup_printf("%s.c", s);
		gtk_entry_set_text (GTK_ENTRY (source_file_widget), t);
		g_free(t);
		g_free(s);
	}
	
}

void on_cc_button_browse_source_clicked (GtkButton *button, GladeXML* gxml) {

	GtkWidget *header_file_widget;
	GtkWidget *source_file_widget;
	const gchar *header_file, *source_file;
	
	source_file = browse_for_file(_("Select source file"));
	
	if (source_file == NULL)
		return;

	header_file_widget = glade_xml_get_widget (gxml, "cc_header_file");
	source_file_widget = glade_xml_get_widget (gxml, "cc_source_file");
	
	header_file = gtk_entry_get_text( GTK_ENTRY(header_file_widget));	
	gtk_entry_set_text (GTK_ENTRY (source_file_widget), source_file);
	
	if(strlen(header_file) == 0) {
		gchar *s = g_strdup(source_file), *t, *p = strrchr(s, '.');
		if (p == NULL) {
			g_free (s);
			return;
		}

		s[strlen(s) - strlen(p)] = '\0';
		t = g_strdup_printf("%s.h", s);
		gtk_entry_set_text (GTK_ENTRY (header_file_widget), t);
		g_free(t);
		g_free(s);
	}	
}

void on_go_button_browse_header_clicked (GtkButton *button, GladeXML* gxml) {
	
	GtkWidget *header_file_widget;
	GtkWidget *source_file_widget;
	const gchar *header_file, *source_file;
	
	/* get the header displaying a browsing window */
	header_file = browse_for_file(_("Select header file"));
	
	if ( header_file == NULL )
		return;

	header_file_widget = glade_xml_get_widget (gxml, "go_header_file");
	source_file_widget = glade_xml_get_widget (gxml, "go_source_file");
	
	source_file = gtk_entry_get_text (GTK_ENTRY (source_file_widget));
	
	/* let's set the header in the entry */
	gtk_entry_set_text(GTK_ENTRY (header_file_widget), header_file);
	
	/* is source_file is void then fill it with a "header_file".c */
	if( strlen (source_file) == 0) {
		gchar *s = g_strdup(header_file), *t, *p = strrchr(s, '.');
		if (p == NULL) {
			g_free (s);
			return;
		}
		s[strlen(s) - strlen(p)] = '\0';
		t = g_strdup_printf("%s.c", s);
		gtk_entry_set_text (GTK_ENTRY (source_file_widget), t);
		g_free(t);
		g_free(s);
	}
}

void on_go_button_browse_source_clicked (GtkButton *button, GladeXML* gxml) {

	GtkWidget *header_file_widget;
	GtkWidget *source_file_widget;
	const gchar *header_file, *source_file;
	
	source_file = browse_for_file(_("Select source file"));
	
	if (source_file == NULL)
		return;

	header_file_widget = glade_xml_get_widget (gxml, "go_header_file");
	source_file_widget = glade_xml_get_widget (gxml, "go_source_file");
	
	header_file = gtk_entry_get_text( GTK_ENTRY(header_file_widget));	
	gtk_entry_set_text (GTK_ENTRY (source_file_widget), source_file);
	
	if( strlen(header_file) == 0 ) {
		gchar *s = g_strdup(source_file), *t, *p = strrchr(s, '.');
		if (p == NULL) {
			g_free (s);
			return;
		}
		s[strlen(s) - strlen(p)] = '\0';
		t = g_strdup_printf("%s.h", s);
		gtk_entry_set_text (GTK_ENTRY (header_file_widget), t);
		g_free(t);
		g_free(s);
	}	
}


void on_create_button_clicked (GtkButton *button, ClassGenData* data) {
	
	GtkWidget *classgen_widget;
	GtkWidget *notebook;
	gint active_page;
	gboolean can_close;
	
	classgen_widget = glade_xml_get_widget (data->gxml, "classgen_main");

	/* check which page is active, whether generic class builder or gobject builder */
	notebook = glade_xml_get_widget (data->gxml, "notebook");
	active_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

	can_close = FALSE;
	
	if (active_page == 0) {				// generic c++ class builder
		can_close = generic_cpp_class_create_code (data);		
	}
	else if (active_page == 1) {		// gobject class builder
		can_close = gobject_class_create_code (data);
	}
	
	if (can_close) {
		DEBUG_PRINT ("going to destroy widget main" );
		gtk_widget_destroy (classgen_widget);
		g_free (data);
	}
}

void on_cancel_button_clicked (GtkButton *button, ClassGenData *data) {
	GtkWidget *classgen_widget;
	
	classgen_widget = glade_xml_get_widget (data->gxml, "classgen_main");
	gtk_widget_destroy (classgen_widget);
	
	g_free (data);
}

void on_inline_toggled (GtkToggleButton *buttom, ClassGenData *data) {
	GtkWidget *cc_inline;
	GtkWidget *source_file_widget;
	GtkWidget *source_button;
	gboolean is_inline;

	source_file_widget = glade_xml_get_widget (data->gxml, "cc_source_file");
	source_button = glade_xml_get_widget (data->gxml, "cc_button_browse_source");
	cc_inline = glade_xml_get_widget (data->gxml, "cc_inline");
	
	is_inline = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cc_inline));
	if(!is_inline) {
		/* set the source file entry and browse buttons sensitive */
		gtk_widget_set_sensitive (source_file_widget, TRUE);
		gtk_widget_set_sensitive (source_button, TRUE);
	}
	else {
		/* set the source file entry and browse buttons insensitive */
		gtk_widget_set_sensitive (source_file_widget, FALSE);
		gtk_widget_set_sensitive (source_button, FALSE);
	}
}

/*----------------------------------------------------------------------------
 * Create the main widget and connect signals to buttons/etc.
 */
 
void
on_classgen_new (AnjutaClassGenPlugin* plugin) {
	GladeXML* gxml;
	GtkWidget *classgen_widget;
	GtkWidget *create_button;
	GtkWidget *cancel_button;
	GtkWidget *cc_button_browse_source;
	GtkWidget *cc_button_browse_header;
	GtkWidget *cc_inheritance;
	GtkWidget *cc_inline;
	GtkWidget *go_button_browse_source;
	GtkWidget *go_button_browse_header;
	ClassGenData *data;
	
	gxml = glade_xml_new (GLADE_FILE, NULL, NULL);
	g_return_if_fail (gxml != NULL );
	
	classgen_widget = glade_xml_get_widget (gxml, "classgen_main" );
	
	cc_button_browse_source = 
					glade_xml_get_widget (gxml, "cc_button_browse_source");
	cc_button_browse_header = 
					glade_xml_get_widget (gxml, "cc_button_browse_header");
	
	cc_inheritance = glade_xml_get_widget (gxml, "cc_inheritance");
	cc_inline = glade_xml_get_widget (gxml, "cc_inline");
	
	go_button_browse_source = 
					glade_xml_get_widget (gxml, "go_button_browse_source");
	go_button_browse_header = 
					glade_xml_get_widget (gxml, "go_button_browse_header");
	
	create_button = glade_xml_get_widget (gxml, "create_button");
	cancel_button = glade_xml_get_widget (gxml, "cancel_button");
	
	gtk_combo_box_set_active (GTK_COMBO_BOX(cc_inheritance), 0);
	
	/* connect signals */
	data = g_new0 (ClassGenData, 1);
	data->gxml = gxml;
	data->plugin = plugin;

	g_signal_connect (G_OBJECT (cc_button_browse_header), "clicked",
		G_CALLBACK (on_cc_button_browse_header_clicked), gxml);
		
	g_signal_connect (G_OBJECT (cc_button_browse_source), "clicked",
		G_CALLBACK (on_cc_button_browse_source_clicked), gxml);

	g_signal_connect (G_OBJECT (go_button_browse_header), "clicked",
		G_CALLBACK (on_go_button_browse_header_clicked), gxml);

	g_signal_connect (G_OBJECT (go_button_browse_source), "clicked",
		G_CALLBACK (on_go_button_browse_source_clicked), gxml);

	g_signal_connect (G_OBJECT (create_button), "clicked",
		G_CALLBACK (on_create_button_clicked), data);

	g_signal_connect (G_OBJECT (cancel_button), "clicked",
		G_CALLBACK (on_cancel_button_clicked), data);
		
	g_signal_connect (G_OBJECT (cc_inline), "toggled",
		G_CALLBACK (on_inline_toggled), data);

	g_signal_connect (G_OBJECT (classgen_widget), "key-press-event",
					  GTK_SIGNAL_FUNC (on_class_gen_key_press_event),
					  data);

	gtk_widget_show (classgen_widget);
}


gboolean
on_class_gen_key_press_event(GtkWidget *widget, GdkEventKey *event,
                                  ClassGenData *data)
{
	if (event->keyval == GDK_Escape)
	{
		GtkWidget *classgen_widget;
	
		classgen_widget = glade_xml_get_widget (data->gxml, "classgen_main");
		gtk_widget_destroy (classgen_widget);
		g_free (data);
		return TRUE;
	}
	return FALSE;
}
