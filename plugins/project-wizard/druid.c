/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    druid.c
    Copyright (C) 2004 Sebastien Granjoux

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
//	All Druid Widget functions

#include <config.h>
#include <gnome.h>
#include <gtk/gtkactiongroup.h>
#include <libgnome/gnome-i18n.h>
#include <libanjuta/interfaces/ianjuta-document-manager.h>
#include <libanjuta/interfaces/ianjuta-wizard.h>
#include <string.h>

#include "plugin.h"
#include "druid.h"
#include "header.h"
#include "property.h"
#include "parser.h"
#include "install.h"
#include "autogen.h"

#define GLADE_FILE PACKAGE_DATA_DIR"/glade/anjuta-project-wizard.glade"
#define PROJECT_WIZARD_DIRECTORY PACKAGE_DATA_DIR"/project"
#define PIXMAP_APPWIZ_LOGO PACKAGE_DATA_DIR"/glade/applogo.png"
#define PIXMAP_APPWIZ_WATERMARK PACKAGE_DATA_DIR"/glade/appwizard.png"

#define NEW_PROJECT_DIALOG "druid_window"
#define PROJECT_SELECTION_LIST "project_list"
#define DRUID_WIDGET "druid"
#define DRUID_START_PAGE "start_page"
#define DRUID_SELECTION_PAGE "selection_page"
#define DRUID_PROPERTY_PAGE "property_page"
#define DRUID_PROPERTY_LABEL "property_label"
#define DRUID_PROPERTY_TABLE "property_table"
#define DRUID_FINISH_PAGE "finish_page"

#define DRUID_DELETE_SIGNAL "on_druid_delete"
#define DRUID_CANCEL_SIGNAL "on_druid_cancel"
#define DRUID_BACK_SIGNAL "on_druid_back"
#define DRUID_NEXT_SIGNAL "on_druid_next"
#define DRUID_FINISH_SIGNAL "on_druid_finish"


// Keep all gui data
struct _NPWDruid
{
	GtkWidget* dialog;
	GnomeIconList* icon_list;
	GnomeDruid* druid;
	const gchar* project_file;
	GnomeDruidPage* selection_page;
	GnomeDruidPageStandard* property_page;
	GtkLabel* property_label;
	GtkTable* property_table;
	GnomeDruidPage* finish_page;
	NPWPlugin* plugin;
	
	guint page;
	GQueue* page_list;
	NPWPageParser* parser;
	NPWHeaderList* header_list;
	NPWHeader* header;
	NPWAutogen* gen;
	gboolean busy;
};

// libGlade doesn't seem to set this parameter for the start page

static void
npw_druid_complete_edge_pages(NPWDruid* this, GladeXML* xml)
{
	GdkColor bg_color = {0, 15616, 33280, 46848};
	GtkWidget* page;
	GdkPixbuf* pixbuf;

	// Start page
	page = glade_xml_get_widget(xml, DRUID_START_PAGE);
	gnome_druid_page_edge_set_bg_color(GNOME_DRUID_PAGE_EDGE(page), &bg_color);
	gnome_druid_page_edge_set_logo_bg_color(GNOME_DRUID_PAGE_EDGE(page), &bg_color);
	pixbuf = gdk_pixbuf_new_from_file(PIXMAP_APPWIZ_WATERMARK, NULL);
	gnome_druid_page_edge_set_watermark(GNOME_DRUID_PAGE_EDGE(page), pixbuf);
	g_object_unref(pixbuf);
	pixbuf = gdk_pixbuf_new_from_file(PIXMAP_APPWIZ_LOGO, NULL);
	gnome_druid_page_edge_set_logo(GNOME_DRUID_PAGE_EDGE(page), pixbuf);

	// Finish page
	page = glade_xml_get_widget(xml, DRUID_FINISH_PAGE);
	gnome_druid_page_edge_set_bg_color(GNOME_DRUID_PAGE_EDGE(page), &bg_color);
	gnome_druid_page_edge_set_logo_bg_color(GNOME_DRUID_PAGE_EDGE(page), &bg_color);
	gnome_druid_page_edge_set_logo(GNOME_DRUID_PAGE_EDGE(page), pixbuf);
	g_object_unref(pixbuf);
}


// Fill icon list in project selection page

static void
cb_druid_insert_project_icon(NPWHeader* head, gpointer data)
{
	gint idx;
	NPWDruid* druid = (NPWDruid*)data;
	
	idx = gnome_icon_list_append(druid->icon_list, npw_header_get_iconfile(head), npw_header_get_name(head));
	gnome_icon_list_set_icon_data(druid->icon_list, idx, head);
}

static gboolean
npw_druid_fill_selection_page(NPWDruid* this)
{
	// Create list of projects
	if (this->header_list != NULL) npw_header_list_destroy(this->header_list);
	this->header_list = npw_header_list_new();	
	if (this->header_list == NULL) return FALSE;

	// Fill list with all project in directory
	npw_header_list_readdir(this->header_list, PROJECT_WIZARD_DIRECTORY);

	npw_header_list_foreach_project(this->header_list, cb_druid_insert_project_icon, this);

	return TRUE;
}

// Fill summary page	
	
typedef struct _NPWPropertyContext
{
	NPWDruid* druid;
	NPWPage* page;
	guint row;
	GString* text;
} NPWPropertyContext;

static void
cb_druid_add_summary_property(NPWProperty* property, gpointer data)
{
	NPWPropertyContext* ctx = (NPWPropertyContext *)data;

	if (npw_property_get_options(property) & NPW_SUMMARY_OPTION)
	{
		g_string_append(ctx->text, _(npw_property_get_label(property)));
		g_string_append(ctx->text, npw_property_get_value(property));
		g_string_append(ctx->text, "\n");
	}
}

static void
npw_druid_fill_summary(NPWDruid* this)
{
	NPWPropertyContext ctx;
	NPWPage* page;
	guint i;
	GString* text;

	text = g_string_new(_("Confim the following information:\n\n"));
	
	g_string_append(text,_("Project Type: "));
	g_string_append(text, _(npw_header_get_name(this->header)));
	g_string_append(text,"\n");

	ctx.druid = this;
	ctx.text = text;
	for (i = 0; (page = g_queue_peek_nth(this->page_list, i)) != NULL; ++i)
	{
		ctx.page = page;
		npw_page_foreach_property(page, cb_druid_add_summary_property, &ctx);
	}

	gnome_druid_page_edge_set_text(GNOME_DRUID_PAGE_EDGE(this->finish_page), text->str);
	g_string_free(text, TRUE);
}

// Fill project property setting page

static void
cb_druid_destroy_widget(GtkWidget* widget, gpointer data)
{
	gtk_widget_destroy(widget);
}

static void
cb_druid_add_property(NPWProperty* property, gpointer data)
{
	GtkWidget* label;
	GtkWidget* entry;
	NPWPropertyContext* ctx = (NPWPropertyContext *)data;
	const gchar* value;

	value = npw_property_get_value(property);
	switch(npw_property_get_type(property))
	{
	case NPW_BOOLEAN_PROPERTY:
		entry = gtk_check_button_new();
		if (value)
		{
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry),
										  (gboolean)atoi(value));
		}
		break;
	case NPW_INTEGER_PROPERTY:
		entry = gtk_spin_button_new (NULL, 1, 0);
		if (value)
		{
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (entry), atoi(value));
		}
		break;
	case NPW_STRING_PROPERTY:
		entry = gtk_entry_new();
		if (value) gtk_entry_set_text(GTK_ENTRY(entry), value);
		break;
	case NPW_DIRECTORY_PROPERTY:
		entry = gnome_file_entry_new(NULL, NULL);
		gnome_file_entry_set_directory_entry(GNOME_FILE_ENTRY(entry), TRUE);
		if (value) gnome_file_entry_set_filename(GNOME_FILE_ENTRY(entry), value);
		break;
	case NPW_FILE_PROPERTY:
		entry = gnome_file_entry_new(NULL, NULL);
		gnome_file_entry_set_directory_entry(GNOME_FILE_ENTRY(entry), FALSE);
		if (value) gnome_file_entry_set_filename(GNOME_FILE_ENTRY(entry), value);
		break;
	default:
		return;
	}

	// Add label and entry
	gtk_table_resize(ctx->druid->property_table, ctx->row + 1, 2);
	label = gtk_label_new(_(npw_property_get_label(property)));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label), 5, 5);
	gtk_table_attach(ctx->druid->property_table, label, 0, 1, ctx->row, ctx->row + 1,
		(GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(0), 0, 0);
	gtk_table_attach(ctx->druid->property_table, entry, 1, 2, ctx->row, ctx->row + 1,
		(GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(0), 0, 0);

	npw_property_set_widget(property, entry);
	ctx->row++;
};

static NPWPage*
npw_druid_add_new_page(NPWDruid* this)
{
	NPWPage* page;

	// Create cache if not here
	if (this->page_list == NULL)
	{
		this->page_list = g_queue_new();
	}
	
	// Get page in cache
	page = g_queue_peek_nth(this->page_list, this->page - 1);
	if (page == NULL)
	{
		// Page not found in cache, create
		page = npw_page_new();

		// Add page in cache
		g_queue_push_tail(this->page_list, page);
	}

	return page;
}

static NPWPage*
npw_druid_remove_current_page(NPWDruid* this)
{
	NPWPage* page;

	this->page--;

	page = g_queue_pop_tail(this->page_list);
	if (page != NULL)
	{
		npw_page_destroy(page);
		npw_autogen_remove_definition(this->gen, page);
	}

	return g_queue_peek_tail(this->page_list);
}

static gboolean
npw_druid_fill_property_page(NPWDruid* this, NPWPage* page)
{
	NPWPropertyContext ctx;

	// Remove previous widgets
	gtk_container_foreach(GTK_CONTAINER(this->property_table), cb_druid_destroy_widget, NULL);

	// Update title	
	gnome_druid_page_standard_set_title(this->property_page, _(npw_page_get_label(page)));
	gtk_label_set_text(this->property_label, _(npw_page_get_description(page)));

	// Add new widget
	ctx.druid = this;
	ctx.page = page;
	ctx.row = 0;
	npw_page_foreach_property(page, cb_druid_add_property, &ctx);

	// Change page only if current is project selection page
	gnome_druid_set_page(this->druid, GNOME_DRUID_PAGE(this->property_page));
	gtk_widget_show_all (this->dialog);

	return TRUE;
}

// Save in page all value entered by user

static void
cb_save_property(NPWProperty* property, gpointer data)
{
	const gchar* value;

	switch(npw_property_get_type(property))
	{
	case NPW_INTEGER_PROPERTY:
		{
			char buff[215];
			gint int_value;
			
			int_value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(npw_property_get_widget(property)));
			snprintf (buff, 215, "%d", int_value);
			value = buff;
		}
		break;
	case NPW_BOOLEAN_PROPERTY:
		{
			char buff[215];
			gint int_value;
			
			int_value = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (npw_property_get_widget(property)));
			snprintf (buff, 215, "%d", int_value);
			value = buff;
		}
		break;
	case NPW_STRING_PROPERTY:
		value = gtk_entry_get_text(GTK_ENTRY(npw_property_get_widget(property)));
		break;
	case NPW_DIRECTORY_PROPERTY:
	case NPW_FILE_PROPERTY:
		value = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(npw_property_get_widget(property)), FALSE);
		break;
	default:
		return;
	}

	npw_property_set_value(property, value);
};

static void
npw_druid_save_all_values(NPWDruid* this)
{
	NPWPropertyContext ctx;
	NPWPage* page;

	page = g_queue_peek_nth(this->page_list, this->page - 2);
	ctx.druid = this;
	ctx.page = page;
	
	npw_page_foreach_property(page, cb_save_property, &ctx);
}

// Clear page in cache up to downto (0 = all pages)

/*static void
clear_page_cache(GQueue* cache, guint downto)
{
	NPWPage* page;

	if (cache == NULL) return;

	while (g_queue_get_length(cache) > downto)
	{
		page = g_queue_pop_tail(cache);
		npw_page_destroy(page);
	}
}*/

static void
npw_destroy_druid(NPWDruid* this)
{
	g_return_if_fail(this != NULL);


	if (this->page_list != NULL)
	{
		NPWPage* page;

		// Delete page list
		while ((page = (NPWPage *)g_queue_pop_tail(this->page_list)) != NULL)
		{
			npw_page_destroy(page);
		}
		g_queue_free(this->page_list);
	}
	if (this->gen != NULL)
	{
		npw_autogen_destroy(this->gen);
	}
	if (this->parser != NULL)
	{
		npw_page_parser_destroy(this->parser);
	}
	npw_header_list_destroy(this->header_list);
	gtk_widget_destroy(this->dialog);
	this->plugin->druid = NULL;
	g_free(this);
}

// Call back

static gboolean
on_druid_cancel(GtkWidget* window, NPWDruid* druid)
{
	npw_destroy_druid(druid);

	return TRUE;
}

static gboolean
on_druid_delete(GtkWidget* window, GdkEvent* event, NPWDruid* druid)
{
	npw_destroy_druid(druid);

	return TRUE;
}

static void
on_druid_get_new_page(NPWAutogen* gen, gpointer data)
{
	NPWDruid* this = (NPWDruid *)data;
	NPWPage* page;

	page = g_queue_peek_nth(this->page_list, this->page - 1);

	if (npw_page_get_name(page) == NULL)
	{
		// no page, display finish page
		npw_druid_fill_summary(this);
		gnome_druid_set_page(this->druid, this->finish_page);
	}
	else
	{
		// display property page
		npw_druid_fill_property_page(this, page);
	}
	this->busy = FALSE;
}

static void
on_druid_parse_page(const gchar* output, gpointer data)
{
	NPWPageParser* parser = (NPWPageParser*)data;
	
	npw_page_parser_parse(parser, output, strlen(output), NULL);
}

static gboolean
on_druid_next(GnomeDruidPage* page, GtkWidget* widget, NPWDruid* this)
{
	// Skip if busy
	if (this->busy) return TRUE;

	this->page++;
	if (this->page == 1)
	{
		// Current is Select project page
		guint idx;
		NPWHeader* header;

		idx = (gint)gnome_icon_list_get_selection(GNOME_ICON_LIST(this->icon_list))->data;
		header = (NPWHeader*)gnome_icon_list_get_icon_data(GNOME_ICON_LIST(this->icon_list), idx);

		this->project_file = npw_header_get_filename(header);
		this->header = header;
		if (this->gen == NULL)
			this->gen = npw_autogen_new();
		npw_autogen_set_input_file(this->gen, this->project_file, "[+","+]");

		npw_autogen_add_default_definition(this->gen, anjuta_shell_get_preferences(ANJUTA_PLUGIN(this->plugin)->shell, NULL));
	}
	else
	{
		// Current is one of the property page
		npw_druid_save_all_values(this);
		npw_autogen_add_definition(this->gen, g_queue_peek_nth(this->page_list, this->page - 2));
	}

	if (this->parser != NULL)
		npw_page_parser_destroy(this->parser);
	this->parser = npw_page_parser_new(npw_druid_add_new_page(this), this->project_file, this->page - 1);
	npw_autogen_set_output_callback(this->gen, on_druid_parse_page, this->parser);
	npw_autogen_execute(this->gen, on_druid_get_new_page, this);
	this->busy = TRUE;

	return TRUE;
}

static gboolean
on_druid_back(GnomeDruidPage* dpage, GtkWidget* widget, NPWDruid* druid)
{
	NPWPage* page;

	// Skip if busy
	if (druid->busy) return TRUE;

	g_return_val_if_fail(druid->page > 0, TRUE);

	page = npw_druid_remove_current_page(druid);
	if (page != NULL)
	{
		// Create property pahe
		npw_druid_fill_property_page(druid, page);
	}
	else
	{
		// Go back to project selection page
		gnome_druid_set_page(druid->druid, druid->selection_page);
	}

	return TRUE;
}

static void
on_druid_finish(GnomeDruidPage* page, GtkWidget* widget, NPWDruid* druid)
{
/*	NPWInstall* inst;

	inst =npw_install_new(druid->page_list);
	druid->page_list = NULL;
	npw_install_start(npw_header_get_filename(druid->header));

	npw_destroy_druid(druid);*/
/*	NPWFileList* list;

	list = npw_file_list_new();
	npw_file_list_read(list, npw_header_get_filename(druid->header));
	npw_file_list_install(list);

	npw_destroy_druid(druid);*/

	NPWInstall* inst;

	inst = npw_install_new(druid->plugin);
	npw_install_set_property(inst, druid->page_list, ANJUTA_PLUGIN(druid->plugin));
	npw_install_set_wizard_file(inst, npw_header_get_filename(druid->header));
	npw_install_launch(inst);

	npw_destroy_druid(druid);
}

static void
npw_druid_connect_all_signal(NPWDruid* this, GladeXML* xml)
{
	glade_xml_signal_connect_data(xml, DRUID_DELETE_SIGNAL, GTK_SIGNAL_FUNC(on_druid_delete), this);
	glade_xml_signal_connect_data(xml, DRUID_CANCEL_SIGNAL, GTK_SIGNAL_FUNC(on_druid_cancel), this);
	glade_xml_signal_connect_data(xml, DRUID_FINISH_SIGNAL, GTK_SIGNAL_FUNC(on_druid_finish), this);
	glade_xml_signal_connect_data(xml, DRUID_NEXT_SIGNAL, GTK_SIGNAL_FUNC(on_druid_next), this);
	glade_xml_signal_connect_data(xml, DRUID_BACK_SIGNAL, GTK_SIGNAL_FUNC(on_druid_back), this);
}

NPWDruid*
npw_druid_new(NPWPlugin* plugin)
{
	GladeXML* xml;
	NPWDruid* this;

	// Skip if already created
	if (plugin->druid != NULL) return plugin->druid;

	this = g_new0(NPWDruid, 1);
	xml = glade_xml_new(GLADE_FILE, NEW_PROJECT_DIALOG, NULL);
	if ((this == NULL) || (xml == NULL))
	{
		anjuta_util_dialog_error(NULL, _("Unable to build project wizard user interface"));
		g_free(this);

		return NULL;
	}

	// Get reference on all useful widget
	this->dialog = glade_xml_get_widget(xml, NEW_PROJECT_DIALOG);
	this->icon_list = GNOME_ICON_LIST(glade_xml_get_widget(xml, PROJECT_SELECTION_LIST));
	this->druid = GNOME_DRUID(glade_xml_get_widget(xml, DRUID_WIDGET));
	this->selection_page = GNOME_DRUID_PAGE(glade_xml_get_widget(xml, DRUID_SELECTION_PAGE));
	this->property_page = GNOME_DRUID_PAGE_STANDARD(glade_xml_get_widget(xml, DRUID_PROPERTY_PAGE));
	this->property_label = GTK_LABEL(glade_xml_get_widget(xml, DRUID_PROPERTY_LABEL));
	this->property_table = GTK_TABLE(glade_xml_get_widget(xml, DRUID_PROPERTY_TABLE));
	this->finish_page = GNOME_DRUID_PAGE(glade_xml_get_widget(xml, DRUID_FINISH_PAGE));
	this->page = 0;
	this->busy = FALSE;
	this->page_list = NULL;
	this->gen = npw_autogen_new();
	this->plugin = plugin;

	npw_druid_complete_edge_pages(this, xml);
	npw_druid_connect_all_signal(this, xml);
	g_object_unref(xml);

	// Needed by GnomeDruid widget
	gtk_widget_show_all (this->dialog);

	npw_druid_fill_selection_page(this);

	plugin->druid = this;

	return this;
}

void
npw_druid_show(NPWDruid* this)
{
	g_return_if_fail(this);

	// Display dialog box
	if (this->dialog)
		gtk_window_present(GTK_WINDOW(this->dialog));
}

