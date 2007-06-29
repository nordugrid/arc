noinst_LTLIBRARIES = libconf.la

libconf_ladir=$(includedir)
libconf_la_HEADERS=environment.h   daemon.h   conf_file.h   conf.h   conf_sections.h   gridmap.h
libconf_la_SOURCES=environment.cpp daemon.cpp conf_file.cpp conf.cpp conf_sections.cpp gridmap.cpp
libconf_la_CPPFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libconf_la_LDFLAGS=../../../../libs/common/libarccommon.la
