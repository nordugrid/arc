bin_PROGRAMS = glitesub glitestat glitekill gliteclean glitedelegate gliteundelegate
#doc_DATA = client.xml jsdl_simple.xml jsdl_stage.xml
#EXTRA_DIST = $(doc_DATA)
EXTRA_DIST = openSSLFunctions.cpp
SOURCES = cream_client.cpp cream_client.h
LIBS = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/misc/libarcclient.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la
commonCXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include $(OPENSSL_CFLAGS)
commonLDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS)
glitesub_SOURCES  = $(SOURCES) glitesub.cpp
glitesub_CXXFLAGS = $(commonCXXFLAGS)
glitesub_LDFLAGS  = $(commonLDFLAGS)
glitesub_LDADD    = $(LIBS)

glitestat_SOURCES  = $(SOURCES) glitestat.cpp
glitestat_CXXFLAGS = $(commonCXXFLAGS)
glitestat_LDFLAGS  = $(commonLDFLAGS)
glitestat_LDADD    = $(LIBS)

glitekill_SOURCES  = $(SOURCES) glitekill.cpp
glitekill_CXXFLAGS = $(commonCXXFLAGS)
glitekill_LDFLAGS  = $(commonLDFLAGS)
glitekill_LDADD    = $(LIBS)

gliteclean_SOURCES  = $(SOURCES) gliteclean.cpp
gliteclean_CXXFLAGS = $(commonCXXFLAGS)
gliteclean_LDFLAGS  = $(commonLDFLAGS)
gliteclean_LDADD    = $(LIBS)

glitedelegate_SOURCES  = $(SOURCES) glitedelegate.cpp
glitedelegate_CXXFLAGS = $(commonCXXFLAGS)
glitedelegate_LDFLAGS  = $(commonLDFLAGS)
glitedelegate_LDADD    = $(LIBS)

gliteundelegate_SOURCES  = $(SOURCES) gliteundelegate.cpp
gliteundelegate_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
gliteundelegate_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
gliteundelegate_LDADD    = $(LIBS)

#man_MANS = apsub.1 apstat.1 apkill.1 apsstat.1 apclean.1
