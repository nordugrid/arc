noinst_LTLIBRARIES = libconf.la

libconf_ladir=$(includedir)
libconf_la_HEADERS=environment.h   daemon.h   conf_file.h   conf.h
libconf_la_SOURCES=environment.cpp daemon.cpp conf_file.cpp conf.cpp
libconf_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libconf_la_LDFLAGS=../../../../libs/common/libarccommon.la
