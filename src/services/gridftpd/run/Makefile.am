noinst_LTLIBRARIES = librun.la

librun_la_SOURCES=run.cpp run.h run_plugin.cpp run_plugin.h
librun_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
librun_la_LIBADD=$(DLOPEN_LIBS) -lpthread
