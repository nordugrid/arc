noinst_LTLIBRARIES = libconf.la

libconf_la_SOURCES=environment.cpp daemon.cpp conf_file.cpp conf.cpp conf_sections.cpp gridmap.cpp conf_pre.cpp conf_map.cpp \
                   environment.h   daemon.h   conf_file.h   conf.h   conf_sections.h   gridmap.h   conf_pre.h   conf_map.h
libconf_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
#libconf_la_LIBADD=$(top_srcdir)/libs/common/libarccommon.la ../misc/libmisc.la ../run/librun.la ../log/liblog.la ../jobs/libjobs.la
