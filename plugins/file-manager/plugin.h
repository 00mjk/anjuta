
#ifndef _FILE_MANAGER_PLUGIN_H_
#define _FILE_MANAGER_PLUGIN_H_

#include <libanjuta/anjuta-plugin.h>

typedef struct _FileManagerPlugin FileManagerPlugin;
typedef struct _FileManagerPluginClass FileManagerPluginClass;

struct _FileManagerPlugin{
	AnjutaPlugin parent;
	
	AnjutaUI *ui;
	AnjutaPreferences *prefs;
	GtkWidget *scrolledwindow;
	GtkWidget *tree;
	gchar *top_dir;
	guint root_watch_id;
};

struct _FileManagerPluginClass{
	AnjutaPluginClass parent_class;
};

void fv_init (FileManagerPlugin *fv);
void fv_finalize (FileManagerPlugin *fv);

void       fv_set_root (FileManagerPlugin *fv, const gchar *root_dir);
void       fv_clear    (FileManagerPlugin *fv);
GList*     fv_get_node_expansion_states (FileManagerPlugin *fv);
void       fv_set_node_expansion_states (FileManagerPlugin *fv,
									  GList *expansion_states);
gchar*     fv_get_selected_file_path (FileManagerPlugin *fv);

void       fv_refresh (FileManagerPlugin *fv);

// void        fv_customize(gboolean really_show);
// gboolean   fv_open_file (const char *path, gboolean use_anjuta);
// void        fv_session_save (ProjectDBase *p);
// void        fv_session_load (ProjectDBase *p);

#endif
