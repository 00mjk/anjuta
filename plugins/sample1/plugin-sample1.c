
#include "../../src/anjuta.h"
#include "../../src/anjuta_info.h"

/* Get module description */
gchar	*GetDescr()
{
	return g_strdup(_("Sample Plugin Description. Holla!"));
}
	/* GetModule Version hi/low word 1.02 0x10002 */
glong
GetVersion()
{
	return 0x10000L ;
}

gboolean
Init( GModule *self, void **pUserData, AnjutaApp* p )
{
	return TRUE ;
}

void
CleanUp( GModule *self, void *pUserData, AnjutaApp* p )
{
}

void
Activate( GModule *self, void *pUserData, AnjutaApp* p)
{
	GList* mesg = NULL;
	mesg = g_list_append (mesg, _("Hello world!."));
	mesg = g_list_append (mesg, _("Sample plugin!"));
	
	anjuta_info_show_list (mesg, 0, 0);
}

gchar
*GetMenuTitle( GModule *self, void *pUserData )
{
	return g_strdup(_("Sample plugin"));
}

gchar
*GetTooltipText( GModule *self, void *pUserData ) 
{
   return g_strdup(_("This is a sample plugin"));
}
