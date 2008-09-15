pkglib_LTLIBRARIES = libdmcgridftp.la

libdmcgridftp_la_SOURCES  = DMCGridFTP.cpp DataPointGridFTP.cpp Lister.cpp \
			    GlobusErrorUtils.cpp GSSCredential.cpp \
                            DMCGridFTP.h   DataPointGridFTP.h   Lister.h \
			    GlobusErrorUtils.h   GSSCredential.h
libdmcgridftp_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
			    $(GLOBUS_FTP_CLIENT_CFLAGS) -I$(top_srcdir)/include
libdmcgridftp_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
			    $(GLOBUS_FTP_CLIENT_LIBS) \
			    $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
			    $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
			    $(top_srcdir)/src/hed/libs/common/libarccommon.la
libdmcgridftp_la_LDFLAGS  = -no-undefined -avoid-version -module
