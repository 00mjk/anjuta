[+ autogen5 template +]
# Sample Makefile for a anjuta plugin.
[+IF (=(get "HasUI") "1")+]
# Plugin UI file
[+NameCLower+]_uidir = $(anjuta_ui_dir)
[+NameCLower+]_ui_DATA =  [+NameHLower+].ui
[+ENDIF+]
[+IF (=(get "HasGladeFile") "1")+]
# Plugin Glade file
[+NameCLower+]_gladedir = $(anjuta_glade_dir)
[+NameCLower+]_glade_DATA =  [+NameHLower+].glade
[+ENDIF+]
# Plugin Icon file
[+NameCLower+]_pixmapsdir = $(anjuta_image_dir)
[+NameCLower+]_pixmaps_DATA = [+NameHLower+].png

# Plugin description file
plugin_in_files = [+NameHLower+].plugin.in
%.plugin: %.plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

[+NameCLower+]_plugindir = $(anjuta_plugin_dir)
[+NameCLower+]_plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

# NOTE :
# The naming convention is very intentional
# We are forced to use the prefix 'lib' by automake and libtool
#    There is probably a way to avoid it but it is not worth to effort
#    to find out.
# The 'anjuta_' prfix is a safety measure to avoid conflicts where the
#    plugin 'libpython.so' needs to link with the real 'libpython.so'

# Include paths
AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DANJUTA_DATA_DIR=\"$(anjuta_data_dir)\" \
	-DANJUTA_PLUGIN_DIR=\"$(anjuta_plugin_dir)\" \
	-DANJUTA_IMAGE_DIR=\"$(anjuta_image_dir)\" \
	-DANJUTA_GLADE_DIR=\"$(anjuta_glade_dir)\" \
	-DANJUTA_UI_DIR=\"$(anjuta_ui_dir)\" \
	-DPACKAGE_DATA_DIR=\"$(datadir)\" \
	-DPACKAGE_SRC_DIR=\"$(srcdir)\" \
	$(LIBANJUTA_CFLAGS) [+IF (=(get "HavePackage") "1")+]$([+NameCUpper+]_CFLAGS)[+ENDIF+]\
	-include config.h # so it gets included before glib-i18n.h

# Where to install the plugin
plugindir = $(anjuta_plugin_dir)

# The plugin
plugin_LTLIBRARIES = lib[+NameHLower+].la

# Plugin sources
lib[+NameCLower+]_la_VALASOURCES = plugin.vala config.vapi
[+NameCLower+]_PACKAGES = --pkg libanjuta-1.0 [+ IF PackageModule1 +][+ % PackageModule1 "--pkg %s" +][+ ENDIF +] [+ IF PackageModule2 +][+ % PackageModule2 "--pkg %s" +][+ ENDIF +] [+ IF PackageModule3 +][+ % PackageModule3 "--pkg %s" +][+ ENDIF +] [+ IF PackageModule4 +][+ % PackageModule4 "--pkg %s" +][+ ENDIF +] [+ IF PackageModule5 +][+ % PackageModule5 "--pkg %s" +][+ ENDIF +]

BUILT_SOURCES = [+NameCLower+].vala.stamp

lib[+NameCLower+]_la_SOURCES = [+NameCLower+].vala.stamp \
                               $(lib[+NameCLower+]_la_VALASOURCES:.vala=.c) \
                               $(lib[+NameCLower+]_la_VALASOURCES:.vala=.h)

[+NameCLower+].vala.stamp : $(lib[+NameCLower+]_la_VALASOURCES)
	$(VALAC) -C $([+NameCLower+]_PACKAGES) $^
	touch $@

# Plugin dependencies
lib[+NameCLower+]_la_LIBADD = \
	$(LIBANJUTA_LIBS) [+IF (=(get "HavePackage") "1")+]$([+NameCUpper+]_LIBS)[+ENDIF+]

EXTRA_DIST = \
	$(plugin_in_files) \
	$([+NameCLower+]_plugin_DATA) \
	[+IF (=(get "HasUI") "1")+]$([+NameCLower+]_ui_DATA)[+ENDIF+] \
	[+IF (=(get "HasGladeFile") "1")+]$([+NameCLower+]_glade_DATA)[+ENDIF+] \
	$([+NameCLower+]_pixmaps_DATA)
