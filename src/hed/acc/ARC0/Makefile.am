pkglib_LTLIBRARIES = libaccARC0.la

libaccARC0_la_SOURCES  = TargetRetrieverARC0.cpp TargetRetrieverARC0.h \
                         SubmitterARC0.cpp SubmitterARC0.h \
                         DescriptorsARC0.cpp \
			 JobControllerARC0.cpp JobControllerARC0.h \
			 FTPControl.cpp FTPControl.h
libaccARC0_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
                         $(GLOBUS_FTP_CONTROL_CFLAGS) \
                         -I$(top_srcdir)/include
libaccARC0_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                         $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                         $(top_srcdir)/src/hed/libs/client/libarcclient.la \
			 $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
                         $(top_srcdir)/src/hed/libs/globusutils/libarcglobusutils.la \
			 $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
                         $(GLOBUS_FTP_CONTROL_LIBS)
libaccARC0_la_LDFLAGS  = -no-undefined -avoid-version -module
