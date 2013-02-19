noinst_PROGRAMS = generator

generator_SOURCES = generator-main.cpp Generator.h Generator.cpp
generator_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(AM_CXXFLAGS)
generator_LDADD = ../libarcdatastaging.la $(GLIBMM_LIBS)
