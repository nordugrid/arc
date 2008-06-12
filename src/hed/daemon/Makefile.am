if WIN32
WIN32_DIR=win32
else
WIN32_DIR=
endif

SUBDIRS = scripts $(WIN32_DIR)

sbin_PROGRAMS = arched
SRC_UNIX = unix/main.cpp unix/daemon.cpp unix/daemon.h options.cpp options.h 
SRC_WIN32 = win32/main.cpp options.cpp options.h
if WIN32
SRC = $(SRC_WIN32)
else
SRC = $(SRC_UNIX)
endif
SOURCES=$(SRC)

arched_SOURCES  = $(SOURCES)
arched_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arched_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS)
arched_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

man_MANS = arched.8
