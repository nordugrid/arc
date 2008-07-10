bin_PROGRAMS = glitesub glitestat glitekill gliteclean glitedelegate gliteundelegate

LIBS = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/client/libarcclient.la $(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/acc/CREAM/libCREAMClient.la
commonCXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/src/hed/acc/CREAM
commonLDFLAGS=$(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS)

glitesub_SOURCES  = glitesub.cpp
glitesub_CXXFLAGS = $(commonCXXFLAGS)
glitesub_LDFLAGS  = $(commonLDFLAGS)
glitesub_LDADD    = $(LIBS)

glitestat_SOURCES  = glitestat.cpp
glitestat_CXXFLAGS = $(commonCXXFLAGS)
glitestat_LDFLAGS  = $(commonLDFLAGS)
glitestat_LDADD    = $(LIBS)

glitekill_SOURCES  = glitekill.cpp
glitekill_CXXFLAGS = $(commonCXXFLAGS)
glitekill_LDFLAGS  = $(commonLDFLAGS)
glitekill_LDADD    = $(LIBS)

gliteclean_SOURCES  = gliteclean.cpp
gliteclean_CXXFLAGS = $(commonCXXFLAGS)
gliteclean_LDFLAGS  = $(commonLDFLAGS)
gliteclean_LDADD    = $(LIBS)

glitedelegate_SOURCES  = glitedelegate.cpp
glitedelegate_CXXFLAGS = $(commonCXXFLAGS)
glitedelegate_LDFLAGS  = $(commonLDFLAGS)
glitedelegate_LDADD    = $(LIBS)

gliteundelegate_SOURCES  = gliteundelegate.cpp
gliteundelegate_CXXFLAGS = $(commonCXXFLAGS)
gliteundelegate_LDFLAGS  = $(commonLDFLAGS)
gliteundelegate_LDADD    = $(LIBS)

man_MANS = glitedelegate.1 glitesub.1 glitestat.1 gliteclean.1 glitekill.1 gliteundelegate.1
