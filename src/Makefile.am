EXTRA_DIST =

EXTRA_DEFINES =
EXTRA_INCLUDES =
EXTRA_LIBS =
EXTRA_FLAGS =
EXTRA_SRC =

AM_CFLAGS = -I$(top_srcdir)/include $(LIBVA_CFLAGS) $(EXTRA_DEFINES)

AM_CPPFLAGS = $(EXTRA_INCLUDES)

lib_LTLIBRARIES = libva_inf.la

libva_inf_la_LDFLAGS = $(EXTRA_FLAGS)

libva_inf_la_SOURCES = va_inf.c $(EXTRA_ENCODE_SRC)

libva_inf_la_LIBADD = $(LIBVA_LIBS) $(LIBVA_DRM_LIBS) $(VA_INF_EXTRA_LIBS)

include_HEADERS = $(top_srcdir)/include/va_inf.h
