SUBDIRS = allowpdp denypdp countpdp simplelistpdp simplelistsh
noinst_PROGRAMS = test
# classload_test 

pkglib_LTLIBRARIES = libarcpdc.la
libarcpdc_la_SOURCES  = ArcPDP.cpp PDPplugin.cpp ArcEvaluator.cpp ArcRequest.cpp ArcRequestItem.cpp ArcAttributeFactory.cpp ArcPolicy.cpp ArcRule.cpp ArcFnFactory.cpp ArcAlgFactory.cpp ArcPDP.h ArcEvaluator.h ArcRequest.h ArcRequestItem.h ArcAttributeFactory.h ArcPolicy.h ArcRule.h PolicyParser.cpp PolicyParser.h PolicyStore.cpp PolicyStore.h ArcFnFactory.h ArcAlgFactory.h 
libarcpdc_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include  -DLIBPATH=\"$(top_srcdir)/src/hed/pdc/.libs\"
libarcpdc_la_LIBADD   = allowpdp/liballowpdp.la denypdp/libdenypdp.la countpdp/libcountpdp.la simplelistpdp/libsimplelistpdp.la simplelistsh/libsimplelistsh.la $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la $(LIBXML2_LIBS)
libarcpdc_la_LDFLAGS  = -no-undefined -avoid-version -module

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

#classload_test_SOURCES = classload_test.cpp
#classload_test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#classload_test_LDFLAGS = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
#classload_test_LDADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

