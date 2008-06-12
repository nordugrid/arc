bin_PROGRAMS=paul_gui

paul_gui.res: paul_gui.rc
	windres.exe -o paul_gui.res -i paul_gui.rc --input-format=rc -O coff
BUILT_SOURCES= paul_gui.res
paul_gui_SOURCES  = paul_gui.cpp paul_gui.h 
paul_gui_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
paul_gui_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS) -mwindows paul_gui.res
paul_gui_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la

EXTRA_DIST=arc.ico arc.pnm
