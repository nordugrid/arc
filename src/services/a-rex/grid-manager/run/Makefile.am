noinst_LTLIBRARIES = librun.la

librun_la_SOURCES = run_parallel.cpp run_parallel.h \
	run_plugin.cpp run_plugin.h \
	run_function.cpp run_function.h \
	run_redirected.cpp run_redirected.h
librun_la_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
librun_la_LIBADD = $(DLOPEN_LIBS)
