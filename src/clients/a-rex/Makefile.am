noinst_PROGRAMS=arex_test apsub apstat apkill
SOURCES = arex_client.cpp arex_client.h

arex_test_SOURCES=$(SOURCES) arex_test.cpp
arex_test_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
arex_test_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
arex_test_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la

apsub_SOURCES=$(SOURCES) apsub.cpp
apsub_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
apsub_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
apsub_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la

apstat_SOURCES=$(SOURCES) apstat.cpp
apstat_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
apstat_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
apstat_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la

apkill_SOURCES=$(SOURCES) apkill.cpp
apkill_CPPFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src/libs -I$(top_srcdir)/src/hed/libs
apkill_LDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS)
apkill_LDADD=$(top_srcdir)/src/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la
