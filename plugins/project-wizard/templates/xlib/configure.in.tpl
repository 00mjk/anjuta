[+ autogen5 template +]
dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(configure.in)
AM_INIT_AUTOMAKE([+NameLower+], [+Version+])
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PATH_XTRA

[+IF (=(get "HaveLangCPP") "1")+]
AC_PROG_CPP
AC_PROG_CXX
[+ENDIF+]

[+IF (=(get "HaveI18n") "1")+]
dnl Set gettext package name
GETTEXT_PACKAGE=[+NameLower+]
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

dnl Add the languages which your application supports here.
ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT
[+ENDIF+]

[+IF (=(get "HaveSharedlib") "1")+]
AM_PROG_LIBTOOL
[+ENDIF+]

[+IF (=(get "HavePackage") "1")+]
PKG_CHECK_MODULES(PACKAGE, [[+PackageModule1+] [+PackageModule2+] [+PackageModule3+] [+PackageModule4+] [+PackageModule5+]])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)
[+ENDIF+]

AC_OUTPUT([
Makefile
src/Makefile
[+IF (=(get "HaveI18n") "1")+]po/Makefile.in[+ENDIF+]
])
