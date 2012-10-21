noinst_LTLIBRARIES = libconf.la

libconf_la_SOURCES = \
  GMConfig.h GMConfig.cpp \
  conf_file.cpp conf_file.h \
  conf.cpp conf.h \
  conf_sections.cpp conf_sections.h \
  conf_map.cpp conf_map.h \
  conf_cache.cpp conf_cache.h \
  conf_staging.cpp conf_staging.h
libconf_la_CXXFLAGS = -I$(top_srcdir)/include \
  $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
