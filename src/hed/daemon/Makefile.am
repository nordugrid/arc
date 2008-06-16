if WIN32
PLATFORM_DIR=win32
else
PLATFORM_DIR=unix
endif

SUBDIRS = scripts $(PLATFORM_DIR)

sbin_PROGRAMS = arched
SRC_UNIX = options.cpp options.h 
SRC_WIN32 = options.cpp options.h
LIB_UNIX = unix/libdaemon.la
LIB_WIN32 = win32/libdaemon.la
if WIN32
SRC = $(SRC_WIN32)
LIB = $(LIB_WIN32)
else
SRC = $(SRC_UNIX)
LIB = $(LIB_UNIX)
endif
SOURCES=$(SRC)
LDADD=$(LIB)

arched_SOURCES  = $(SOURCES)
arched_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
arched_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS)
arched_LDADD    = $(LDADD) $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la

man_MANS = arched.8
