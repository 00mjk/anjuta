/*
    session.c
    Copyright (C) 2001  lb

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include "anjuta.h"
#include "utilities.h"
#include "session.h"


/* Id2String */
const gchar	*SessionSectionString( const SessionSectionTypes p_Session )
{
	switch(p_Session)
	{
	default:
		return "LOST+FOUND"; break;
	case SECTION_FILELIST:
		return "filelist"; break;
	case SECTION_FILENUMBER:
		return "filenumbers"; break;
	case SECTION_BREAKPOINTS:
		return "breakpoints"; break;
	case SECTION_FILEMARKERS:
		return "filemarkers"; break;
	case SECTION_FINDTEXT:
		return "find_text"; break;
	case SECTION_REPLACETEXT:
		return "replace_text"; break;
	case SECTION_PROGRAM_ARGUMENTS:
		return "program_arguments"; break;
	case SECTION_FIND_IN_FILES:
		return "find_in_files"; break;
	}
}


void write_config(void)
{
	//gnome_config_private_sync_file("anjuta");
	gnome_config_sync();
}
gchar *GetProfileString( const gchar* szSection, const gchar* szItem, const gchar* szDefault )
{
	gchar	szBuf[256];
	g_snprintf( szBuf, 255, "/anjuta/%s/%s=%s", szSection, szItem, szDefault );
	return gnome_config_private_get_string( szBuf );
	//return gnome_config_get_string( szBuf );
}

gboolean GetProfileBool( const gchar* szSection, const gchar* szItem, const gboolean bDefault )
{
	gchar	szBuf[256];
	g_snprintf( szBuf, 255, "/anjuta/%s/%s=%s", szSection, szItem, bDefault ? "true" : "false" );
	return gnome_config_private_get_bool( szBuf );
	//return gnome_config_get_bool( szBuf );
}


gint GetProfileInt( const gchar* szSection, const gchar* szItem, const gint iDefault )
{
	gchar	szBuf[256];
	g_snprintf( szBuf, 255, "/anjuta/%s/%s=%d", szSection, szItem, iDefault );
	return gnome_config_private_get_int( szBuf );
	//return gnome_config_get_int( szBuf );
}

gboolean WriteProfileString( const gchar* szSection, const gchar* szItem, const gchar* szValue )
{
	gchar	szBuf[256];
	g_snprintf( szBuf, 255, "/anjuta/%s/%s", szSection, szItem );
	gnome_config_private_set_string( szBuf, szValue );
	//gnome_config_set_string( szBuf, szValue );
	return TRUE ;
}

gboolean WriteProfileBool( const gchar* szSection, const gchar* szItem, const gboolean bValue )
{
	gchar	szBuf[256];
	g_snprintf( szBuf, 255, "/anjuta/%s/%s", szSection, szItem );
	gnome_config_private_set_bool( szBuf, bValue );
	//gnome_config_set_bool( szBuf, bValue );
	return TRUE ;
}

gboolean WriteProfileInt( const gchar* szSection, const gchar* szItem, const gint iValue )
{
	gchar	szBuf[256];
	g_snprintf( szBuf, 255, "/anjuta/%s/%s", szSection, szItem );
	gnome_config_private_set_int( szBuf, iValue );
	//gnome_config_set_int( szBuf, iValue );
	return TRUE ;

}

/* Sessione */

gchar * 
GetSessionFile( ProjectDBase * p )
{
	gchar *szDir, *szProject, *szReturn ;
	g_return_val_if_fail( (NULL != p), NULL );
	g_return_val_if_fail( p->project_is_open, NULL );
	szDir = project_dbase_get_dir(p);
	szProject = project_dbase_get_name (p);
	szReturn  = g_strdup_printf( "=%s/%s.pws=", szDir, szProject );
	g_free( szProject );
	return szReturn ;
}

void
session_clear_section( ProjectDBase * p, const gchar *szSection )
{
	gchar *szFile = GetSessionFile( p );
	gchar *szSect = NULL ;
	g_return_if_fail( (NULL != szSection) );
	/* Now appends the item and the specific data... */
	szSect = g_strdup_printf( "%s/%s", szFile, szSection );
	if( NULL !=  szSect )
	{
		gnome_config_clean_section(szSect);
	}
	g_free( szFile );
	g_free( szSect );
}


void
session_save_long_n( ProjectDBase * p, const gchar *szSection, const gint nItem, const glong lValue )
{
	gchar *szFile = GetSessionFile( p );
	gchar *szSect = NULL ;
	g_return_if_fail( (NULL != szSection) );
	/* Now appends the item and the specific data... */
	szSect = g_strdup_printf( "%s/%s/%d", szFile, szSection, nItem );
	if( NULL !=  szSect )
	{
		gnome_config_set_int( szSect, lValue );
	}
	g_free( szFile );
	g_free( szSect );
}

void
session_save_string( ProjectDBase * p, const gchar *szSection, const gint nItem, const gchar *szValue )
{
	gchar *szFile = GetSessionFile( p );
	gchar *szSect = NULL ;
	g_return_if_fail( (NULL != szSection) );
	/* Now appends the item and the specific data... */
	szSect = g_strdup_printf( "%s/%s/%d", szFile, szSection, nItem );
	g_free( szFile );
	if( NULL !=  szSect )
	{
		gnome_config_set_string( szSect, szValue );
	}
	g_free( szSect );	
}

glong
session_get_long_n( ProjectDBase * p, const gchar *szSection, const gint nItem, const glong lValue )
{
	gchar *szFile = GetSessionFile( p );
	gchar *szSect = NULL ;
	glong	gRet = lValue ;
	g_return_val_if_fail( (NULL != szSection), lValue );
	/* Now appends the item and the specific data... */
	szSect = g_strdup_printf( "%s/%s/%d=%ld", szFile, szSection, nItem, lValue );
	g_free( szFile );
	if( NULL !=  szSect )
	{
		gRet = (glong)gnome_config_get_int( szSect );
	}
	g_free( szSect );	
	return gRet ;
}

gchar*
session_get_string_n( ProjectDBase * p, const gchar *szSection, const gint nItem, const gchar *szValue )
{
	gchar	*szFile = GetSessionFile( p );
	gchar	*szSect = NULL ;
	gchar	*szRet = NULL ;
	g_return_val_if_fail( (NULL != szSection), g_strdup(szValue) );
	/* Now appends the item and the specific data... */
	szSect = g_strdup_printf( "%s/%s/%d=%s", szFile, szSection, nItem, szValue );
	g_free( szFile );
	if( NULL !=  szSect )
	{
		szRet = gnome_config_get_string( szSect );
	}
	g_free( szSect );	
	return szRet ;
}



void
session_sync(void)
{
	gnome_config_sync();
}

void
session_get_strings( ProjectDBase * p, const gchar *szSection, gint *pnItems, gchar*** argvp )
{
	gchar *szFile = GetSessionFile( p );
	gchar *szSect = NULL ;
	
	*pnItems = 0 ;
	
	g_return_if_fail( (NULL != szSection) );
	/* Now appends the item and the specific data... */
	szSect = g_strdup_printf( "%s/%s", szFile, szSection );
	if( NULL !=  szSect )
	{
		gnome_config_get_vector( szSect, pnItems, argvp );
	}
	g_free( szFile );
	g_free( szSect );
}

gpointer
session_get_iterator( ProjectDBase * p, const gchar *szSection )
{
	gchar *szFile = GetSessionFile( p );
	gchar *szSect = NULL ;
	gpointer	gIterator = NULL ;
	
	g_return_val_if_fail( (NULL != szSection), NULL );
	/* Now appends the item and the specific data... */
	szSect = g_strdup_printf( "%s/%s", szFile, szSection );
	if( NULL !=  szSect )
	{
		gIterator = gnome_config_init_iterator(szSect);
	}

	g_free( szFile );
	g_free( szSect );
	return gIterator ;
}

/*---------------------------------------------------------------*/

void
save_session_strings( ProjectDBase *p, const gchar *szSession, GList *pLStrings )
{
	g_return_if_fail( NULL != p );
	g_return_if_fail( NULL != szSession );
	
	session_clear_section( p, szSession );
	if( pLStrings )
	{
		const gchar *szFnd ;
		int 		i;
		int			nLen = g_list_length (pLStrings); 
		for (i = 0; i < nLen ; i++)
		{
			szFnd = (gchar*)g_list_nth (pLStrings, i)->data ;
			/*gchar *WriteBufS( gchar* szDst, const gchar* szVal );*/
			if( szFnd && szFnd[0] )
				session_save_string( p, szSession, i, szFnd );
		}
	}
}


GList*
load_session_strings(ProjectDBase * p, const gchar *szSection, GList *pList )
{
	gpointer	config_iterator;
	g_return_val_if_fail( p != NULL, NULL );

	free_string_list ( pList );
	pList = NULL ;

	config_iterator = session_get_iterator( p, szSection );
	if ( config_iterator !=  NULL )
	{
		gchar * szItem, *szData;
		while ((config_iterator = gnome_config_iterator_next( config_iterator,
									&szItem, &szData )))
		{
			if( ( NULL != szData ) )
			{
				pList = g_list_prepend (pList, g_strdup (szData) );
			}
			g_free( szItem );
			g_free( szData );
		}
	}
	return pList ;
}

