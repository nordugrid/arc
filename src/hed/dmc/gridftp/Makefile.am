pkglib_LTLIBRARIES = libdmcgridftp.la

libdmcgridftp_la_SOURCES  = DMCGridFTP.cpp DataPointGridFTP.cpp Lister.cpp \
			    GlobusErrorUtils.cpp GSSCredential.cpp \
                            DMCGridFTP.h   DataPointGridFTP.h   Lister.h \
			    GlobusErrorUtils.h   GSSCredential.h
libdmcgridftp_la_CXXFLAGS = -I$(top_srcdir)/include \
			    $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
			    $(GLOBUS_FTP_CLIENT_CFLAGS)
libdmcgridftp_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
			    $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
			    $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
			    $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
			    $(GLOBUS_FTP_CLIENT_LIBS)
libdmcgridftp_la_LDFLAGS  = -no-undefined -avoid-version -module
