noinst_LTLIBRARIES = libconf.la

libconf_la_SOURCES=conf_vo.cpp daemon.cpp environment.cpp conf.cpp conf_sections.cpp gridmap.cpp conf_map.cpp \
                   conf_vo.h   daemon.h   environment.h   conf.h   conf_sections.h   gridmap.h   conf_map.h
libconf_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_IO_CFLAGS) -I$(top_srcdir)/include
