/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */

#ifndef __ANJUTA_GLUE_PLUGIN_H__
#define __ANJUTA_GLUE_PLUGIN_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define ANJUTA_GLUE_TYPE_PLUGIN            (anjuta_glue_plugin_get_type ())
#define ANJUTA_GLUE_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANJUTA_GLUE_TYPE_PLUGIN, AnjutaGluePlugin))
#define ANJUTA_GLUE_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANJUTA_GLUE_TYPE_PLUGIN, AnjutaGluePluginClass))
#define ANJUTA_GLUE_IS_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANJUTA_GLUE_TYPE_PLUGIN))
#define ANJUTA_GLUE_IS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), ANJUTA_GLUE_TYPE_PLUGIN))
#define ANJUTA_GLUE_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), ANJUTA_GLUE_TYPE_PLUGIN, AnjutaGluePluginClass))

typedef struct _AnjutaGluePlugin      		AnjutaGluePlugin;
typedef struct _AnjutaGluePluginClass 		AnjutaGluePluginClass;
typedef struct _AnjutaGluePluginRegister	AnjutaGluePluginRegister;
typedef struct _AnjutaGluePluginComponent	AnjutaGluePluginComponent;

typedef GType (* AnjutaGluePluginComponentGetTypeFunc) (AnjutaGluePlugin *plugin);

struct _AnjutaGluePlugin
{
	GTypeModule parent;
	
	char *path;
	gboolean loaded;
	
	gboolean resident;
	
	AnjutaGluePluginRegister *type;
};

struct _AnjutaGluePluginClass
{
	GTypeModuleClass parent_class;
	
	gchar* (*build_component_path) (AnjutaGluePlugin* plugin, const gchar *path, const gchar *plugin_name);
	GType (*get_component_type) (AnjutaGluePlugin* plugin, GType parent, const gchar *type_name);
};

struct _AnjutaGluePluginRegister
{
	char *name;
	GType parent;
	AnjutaGluePluginRegister *next;
};

struct _AnjutaGluePluginComponent
{
	const gchar *name;
	AnjutaGluePluginComponentGetTypeFunc get_type_func;
};

GType			anjuta_glue_plugin_get_type				(void) G_GNUC_CONST;

AnjutaGluePlugin* anjuta_glue_plugin_new				(void);

const gchar*	anjuta_glue_plugin_build_component_path	(AnjutaGluePlugin *plugin, const gchar *path, const gchar* component_name);
GType			anjuta_glue_plugin_get_component_type	(AnjutaGluePlugin *plugin, GType parent, const gchar* type_name);
gboolean		anjuta_glue_plugin_set_resident			(AnjutaGluePlugin *plugin, gboolean resident);

#define ANJUTA_GLUE_PLUGIN_MODULE(components) \
ANJUTA_GLUE_REGISTER_COMPONENTS(components) \
ANJUTA_GLUE_GET_COMPONENT_TYPE(components)

#define ANJUTA_GLUE_REGISTER_COMPONENTS(components) 	\
G_MODULE_EXPORT void				\
anjuta_glue_register_components (AnjutaGluePlugin *plugin)	\
{						\
  int i;					\
						\
  for (i = 0; components[i].name != NULL; i++)	\
    {						\
      (* components[i].get_type_func) (plugin);	\
    }						\
}

#define ANJUTA_GLUE_GET_COMPONENT_TYPE(components)	\
G_MODULE_EXPORT GType \
anjuta_glue_get_component_type (AnjutaGluePlugin *plugin, const char *name) \
{ \
  int i; \
\
  for (i = 0; components[i].name != NULL; i++) \
    { \
      if (strcmp (name, components[i].name) == 0) \
	return (* components[i].get_type_func) (plugin); \
    } \
\
  return G_TYPE_INVALID; \
} 

G_END_DECLS

#endif /* __ANJUTA_GLUE_PLUGIN_H__ */
