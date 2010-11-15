pkglib_LTLIBRARIES = libaccJobDescriptionParser.la

libaccJobDescriptionParser_la_SOURCES  = ARCJSDLParser.cpp ARCJSDLParser.h \
                                         JDLParser.cpp JDLParser.h \
                                         XRSLParser.cpp XRSLParser.h \
                                         RSLParser.cpp RSLParser.h \
                                         DescriptorsJobDescriptionParser.cpp
libaccJobDescriptionParser_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
                                        -I$(top_srcdir)/include
libaccJobDescriptionParser_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
                                         $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                                         $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                                         $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                                         $(top_srcdir)/src/hed/libs/message/libarcmessage.la
libaccJobDescriptionParser_la_LDFLAGS  = -no-undefined -avoid-version -module
