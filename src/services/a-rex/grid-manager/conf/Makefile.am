noinst_LTLIBRARIES = libconf.la libenv.la

libconf_la_SOURCES = \
	environment.cpp environment.h \
	conf_file.cpp conf_file.h \
	conf.cpp conf.h \
	conf_sections.cpp conf_sections.h \
	gridmap.cpp gridmap.h \
	conf_pre.cpp conf_pre.h \
	conf_map.cpp conf_map.h \
	conf_cache.cpp conf_cache.h \
	conf_staging.cpp conf_staging.h
libconf_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)

libenv_la_SOURCES = \
	environment.cpp environment.h \
	conf.cpp conf.h \
	conf_sections.cpp conf_sections.h \
	gridmap.cpp gridmap.h \
	conf_pre.cpp conf_pre.h
libenv_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
