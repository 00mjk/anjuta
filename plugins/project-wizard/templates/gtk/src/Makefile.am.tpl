[+ autogen5 template +]
## Process this file with automake to produce Makefile.in

## Created by Anjuta

[+IF (=(get "HaveGlade") "1")+]
gladedir = $(datadir)/[+NameLower+]/glade
glade_DATA = [+NameLower+].glade
[+ENDIF+]

INCLUDES = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(PACKAGE_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = [+NameLower+]

[+NameCLower+]_SOURCES = \
	support.c \
	support.h \
	callbacks.c \
	callbacks.h \
	interface.c\
	interface.h \
	main.c

[+NameCLower+]_LDFLAGS = 

[+NameCLower+]_LDADD = $(PACKAGE_LIBS)

[+IF (=(get "HaveGlade") "1")+]
EXTRA_DIST = $(glade_DATA)
[+ENDIF+]
