/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * preferences.h
 * Copyright (C) 2000 - 2003  Naba Kumar  <naba@gnome.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _ANJUTA_PREFERENCES_H_
#define _ANJUTA_PREFERENCES_H_

#include <gnome.h>
#include <glade/glade.h>

#include <libanjuta/properties.h>
#include <libanjuta/anjuta-preferences-dialog.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
	ANJUTA_PROPERTY_OBJECT_TYPE_TOGGLE,
	ANJUTA_PROPERTY_OBJECT_TYPE_SPIN,
	ANJUTA_PROPERTY_OBJECT_TYPE_ENTRY,
	ANJUTA_PROPERTY_OBJECT_TYPE_MENU,
	ANJUTA_PROPERTY_OBJECT_TYPE_TEXT,
	ANJUTA_PROPERTY_OBJECT_TYPE_COLOR,
	ANJUTA_PROPERTY_OBJECT_TYPE_FONT
} AnjutaPropertyObjectType;

typedef enum
{
	ANJUTA_PROPERTY_DATA_TYPE_BOOL,
	ANJUTA_PROPERTY_DATA_TYPE_INT,
	ANJUTA_PROPERTY_DATA_TYPE_TEXT,
	ANJUTA_PROPERTY_DATA_TYPE_COLOR,
	ANJUTA_PROPERTY_DATA_TYPE_FONT
} AnjutaPropertyDataType;

typedef enum
{
	ANJUTA_PREFERENCES_FILTER_NONE = 0,
	ANJUTA_PREFERENCES_FILTER_PROJECT = 1
} AnjutaPreferencesFilterType;

typedef struct _AnjutaProperty AnjutaProperty;

/* Gets the widget associated with the property */
GtkWidget* anjuta_property_get_widget (AnjutaProperty *prop);

#define ANJUTA_TYPE_PREFERENCES        (anjuta_preferences_get_type ())
#define ANJUTA_PREFERENCES(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), ANJUTA_TYPE_PREFERENCES, AnjutaPreferences))
#define ANJUTA_PREFERENCES_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), ANJUTA_TYPE_PREFERENCES, AnjutaPreferencesClass))
#define ANJUTA_IS_PREFERENCES(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), ANJUTA_TYPE_PREFERENCES))
#define ANJUTA_IS_PREFERENCES_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), ANJUTA_TYPE_PREFERENCES))

typedef struct _AnjutaPreferences        AnjutaPreferences;
typedef struct _AnjutaPreferencesClass   AnjutaPreferencesClass;
typedef struct _AnjutaPreferencesPriv    AnjutaPreferencesPriv;

struct _AnjutaPreferences
{
	AnjutaPreferencesDialog parent;
	
	/*< public >*/
	
	/* Built in values */
	PropsID props_built_in;
	
	/* System values */
	PropsID props_global;
	
	/* User values */ 
	PropsID props_local;
	
	/* Session values */
	PropsID props_session;
	
	/* Instance values */
	PropsID props;
	
	/*< private >*/
	AnjutaPreferencesPriv *priv;
};

struct _AnjutaPreferencesClass
{
	AnjutaPreferencesDialogClass parent;
	void (*changed) (GtkWidget *pref);
};

typedef gboolean (*AnjutaPreferencesCallback) (AnjutaPreferences *pr,
											   const gchar *key,
											   gpointer data);

GType anjuta_preferences_get_type (void);

GtkWidget *anjuta_preferences_new (void);

void anjuta_preferences_add_page (AnjutaPreferences* pr, GladeXML *gxml,
								  const char* glade_widget_name,
								  const gchar *icon_filename);

/*
 * Registers all properties defined for widgets below the 'parent' widget
 * in the given gxml glade UI tree
 */
void anjuta_preferences_register_all_properties_from_glade_xml (AnjutaPreferences* pr,
																GladeXML *gxml,
																GtkWidget *parent);
gboolean
anjuta_preferences_register_property_from_string (AnjutaPreferences *pr,
												  GtkWidget *object,
												  const gchar *property_desc);

gboolean
anjuta_preferences_register_property_raw (AnjutaPreferences *pr, GtkWidget *object,
										  const gchar *key,
										  const gchar *default_value,
										  guint flags,
										  AnjutaPropertyObjectType object_type,
										  AnjutaPropertyDataType  data_type);

gboolean
anjuta_preferences_register_property_custom (AnjutaPreferences *pr,
											 GtkWidget *object,
										     const gchar *key,
										     const gchar *default_value,
										     guint flags,
		void    (*set_property) (AnjutaProperty *prop, const gchar *value),
		gchar * (*get_property) (AnjutaProperty *));

void anjuta_preferences_reset_defaults (AnjutaPreferences *pr);

gboolean anjuta_preferences_save (AnjutaPreferences *pr, FILE *stream);

/* Save excluding the filtered properties. This will save only those
 * properties which DOES NOT have the flags set to values given by the filter.
 */
gboolean anjuta_preferences_save_filtered (AnjutaPreferences *pr, FILE *stream,
										   AnjutaPreferencesFilterType filter);

/* Calls the callback function for each of the properties with the flags
 * matching with the given filter 
 */
void anjuta_preferences_foreach (AnjutaPreferences *pr,
								 AnjutaPreferencesFilterType filter,
								 AnjutaPreferencesCallback callback,
								 gpointer data);

/* This will transfer all the properties values from the main
properties database to the parent session properties database */
void anjuta_preferences_sync_to_session (AnjutaPreferences *pr);

/* Sets the value (string) of a key */
void anjuta_preferences_set (AnjutaPreferences *pr,
							 const gchar *key,
							 const gchar *value);

/* Sets the value (int) of a key */
void anjuta_preferences_set_int (AnjutaPreferences *pr,
								 const gchar *key,
								 const gint value);

/* Gets the value (string) of a key */
/* Must free the return string */
gchar * anjuta_preferences_get (AnjutaPreferences *pr, const gchar *key);

/* Gets the value (int) of a key. If not found, 0 is returned */
gint anjuta_preferences_get_int (AnjutaPreferences *pr, const gchar *key);

/* Gets the value (int) of a key. If not found, the default_value is returned */
gint anjuta_preferences_get_int_with_default (AnjutaPreferences* pr,
											  const gchar *key,
											  gint default_value);

gchar * anjuta_preferences_default_get (AnjutaPreferences *pr,
										const gchar *key);

/* Gets the value (int) of a key */
gint anjuta_preferences_default_get_int (AnjutaPreferences *pr,
										 const gchar *key);

#ifdef __cplusplus
};
#endif

#endif
