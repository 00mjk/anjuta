#ifndef AN_FILE_VIEW_H
#define AN_FILE_VIEW_H

#include <stdio.h>
#include <gtk/gtk.h>

#include "tm_tagmanager.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _AnFileView {
	GtkWidget *win;
	GtkWidget *tree;
	gchar *top_dir;

	TMFileEntry *file_tree;
	TMFileEntry *curr_entry;

	struct {
		GtkWidget *top;
		GtkWidget *open;
		GtkWidget *view;

		struct {
			GtkWidget *top;
			GtkWidget *update;
			GtkWidget *commit;
			GtkWidget *status;
			GtkWidget *log;
			GtkWidget *add;
			GtkWidget *remove;
			GtkWidget *diff;
		} cvs;

		GtkWidget *refresh;
		GtkWidget *customize;
	} menu;
} AnFileView;

AnFileView *fv_populate (gboolean full);
void	    fv_clear (void);
void        fv_customize(gboolean really_show);
gboolean    anjuta_fv_open_file (const char *path, gboolean use_anjuta);

void        fv_session_save (ProjectDBase *p);
void        fv_session_load (ProjectDBase *p);

GList*      fv_get_node_expansion_states (void);
void        fv_set_node_expansion_states (GList *expansion_states);

#ifdef __cplusplus
}
#endif

#endif /* AN_FILE_VIEW_H */
