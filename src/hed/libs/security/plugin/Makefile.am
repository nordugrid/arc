SUBDIRS = 
noinst_PROGRAMS = test
# classload_test 

pkglib_LTLIBRARIES=libsecurityplugin.la
libsecurityplugin_la_SOURCES=SimpleListAuthZ.cpp SimpleListPDP.cpp ArcPDP.cpp PDPplugin.cpp ArcEvaluator.cpp ArcRequest.cpp ArcRequestItem.cpp ArcAttributeFactory.cpp ArcPolicy.cpp ArcRule.cpp ArcFnFactory.cpp ArcAlgFactory.cpp SimpleListAuthZ.h SimpleListPDP.h  ArcPDP.h ArcEvaluator.h ArcRequest.h ArcRequestItem.h ArcAttributeFactory.h ArcPolicy.h ArcRule.h PolicyParser.cpp PolicyParser.h PolicyStore.cpp PolicyStore.h ArcFnFactory.h ArcAlgFactory.h 
libsecurityplugin_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libsecurityplugin_la_LIBADD=$(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/security/libsecurity.la
libsecurityplugin_la_LDFLAGS = -no-undefined -avoid-version -module

test_SOURCES = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/security/api/libsecurityapi.la

#classload_test_SOURCES = classload_test.cpp
#classload_test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#classload_test_LDFLAGS = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
#classload_test_LDADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/security/api/libsecurityapi.la

