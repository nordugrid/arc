lib_LTLIBRARIES=libdmcgridftp.la

libdmcgridftp_la_SOURCES = DMCGridFTP.cpp DataPointGridFTP.cpp
libdmcgridftp_la_CPPFLAGS = ${LIBXML2_CFLAGS} ${GLIBMM_CFLAGS} ${GLOBUS_FTP_CLIENT_CFLAGS} -I$(top_srcdir)/src/libs
libdmcgridftp_la_LIBADD = ${LIBXML2_LIBS} ${GLIBMM_LIBS} ${GLOBUS_FTP_CLIENT_LIBS}
libdmcgridftp_la_LDFLAGS = -no-undefined -avoid-version -module
