/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * git-index-refreshable.h
 * Copyright (C) 2013 James Liggett <jrliggett@cox.net>
 *
 * anjuta is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GIT_INDEX_REFRESHABLE_H_
#define _GIT_INDEX_REFRESHABLE_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <libanjuta/interfaces/ianjuta-refreshable.h>
#include "plugin.h"
#include "git-refresh-index-command.h"

G_BEGIN_DECLS

#define GIT_TYPE_INDEX_REFRESHABLE             (git_index_refreshable_get_type ())
#define GIT_INDEX_REFRESHABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIT_TYPE_INDEX_REFRESHABLE, GitIndexRefreshable))
#define GIT_INDEX_REFRESHABLE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GIT_TYPE_INDEX_REFRESHABLE, GitIndexRefreshableClass))
#define GIT_IS_INDEX_REFRESHABLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIT_TYPE_INDEX_REFRESHABLE))
#define GIT_IS_INDEX_REFRESHABLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GIT_TYPE_INDEX_REFRESHABLE))
#define GIT_INDEX_REFRESHABLE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GIT_TYPE_INDEX_REFRESHABLE, GitIndexRefreshableClass))

typedef struct _GitIndexRefreshableClass GitIndexRefreshableClass;
typedef struct _GitIndexRefreshable GitIndexRefreshable;
typedef struct _GitIndexRefreshablePrivate GitIndexRefreshablePrivate;


struct _GitIndexRefreshableClass
{
	GObjectClass parent_class;
};

struct _GitIndexRefreshable
{
	GObject parent_instance;

	GitIndexRefreshablePrivate *priv;
};

GType git_index_refreshable_get_type (void) G_GNUC_CONST;
IAnjutaRefreshable *git_index_refreshable_new (Git *plugin);

G_END_DECLS

#endif /* _GIT_INDEX_REFRESHABLE_H_ */

