bin_PROGRAMS=paul_gui
noinst_LTLIBRARIES= libdaemon.la

paul_gui.res: paul_gui.rc
	windres.exe -o paul_gui.res -i paul_gui.rc --input-format=rc -O coff
BUILT_SOURCES= paul_gui.res
paul_gui_SOURCES  = paul_gui.cpp paul_gui.h 
paul_gui_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
paul_gui_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS) -mwindows paul_gui.res
paul_gui_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la

libdaemon_la_SOURCES  = main_win32.cpp
libdaemon_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libdaemon_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la
libdaemon_la_LDFLAGS  = $(LIBXML2_LIBS)

EXTRA_DIST=arc.ico arc.pnm
