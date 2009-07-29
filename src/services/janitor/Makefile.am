SUBDIRS = 
DIST_SUBDIRS = 
pkglib_LTLIBRARIES = libjanitor.la
noinst_PROGRAMS = 

initddir = @initddir@
initd_SCRIPTS = 

docdir = @docdir@
doc_DATA = 

# Hardcoded path below. This is there only till I 
# find out how to discover it (A.K.)
PERL_PREFIX = /usr/lib/perl5/5.8.8/i486-linux
PERL_CFLAGS = -I$(PERL_PREFIX)/CORE
PERL_LIBS = -L$(PERL_PREFIX)/CORE -lperl $(PERL_PREFIX)/auto/DynaLoader/DynaLoader.a -ldl

libjanitor_la_SOURCES  = JanitorWebService.cpp JanitorWebService.h TaskQueue.cpp TaskQueue.h TaskSet.cpp TaskSet.h Task.cpp Task.h PerlProcessor.cpp PerlProcessor.h
libjanitor_la_CXXFLAGS = $(PERL_CFLAGS) $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libjanitor_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(GLIBMM_LIBS) $(PERL_LIBS) -lcrypt -lpthread
libjanitor_la_LDFLAGS  = -no-undefined -avoid-version -module

sysconf_DATA = 

EXTRA_DIST = $(doc_DATA)
