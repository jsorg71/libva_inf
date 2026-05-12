# Process this file with autoconf to produce a configure script

AC_PREREQ(2.59)
AC_INIT([yami_inf], [0.1.0], [jay.sorg@gmail.com])
AC_CONFIG_HEADERS(config_ac.h:config_ac-h.in)
AM_INIT_AUTOMAKE([1.6 foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AC_PROG_CC
AC_C_CONST
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([LIBVA], [libva >= 1.3.0], [], [AC_MSG_ERROR([can not find libva])])
PKG_CHECK_MODULES([LIBVA_DRM], [libva-drm >= 1.3.0], [], [AC_MSG_ERROR([can not find libva-drm])])

AM_CONDITIONAL(GOT_PREFIX, test "x${prefix}" != "xNONE")

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
