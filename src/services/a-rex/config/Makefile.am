noinst_LTLIBRARIES = libconfig.la

GRIDMANAGER_LIBS = grid-manager/conf/libconf.la grid-manager/jobs/libjobs.la grid-manager/files/libfiles.la grid-manager/run/librun.la grid-manager/jobdesc/libjobdesc.la grid-manager/misc/libmisc.la grid-manager/log/liblog.la grid-manager/url/liburl.la grid-manager/mail/libmail.la grid-manager/jobdesc/jsdl/libnamespace.la

libconfig_la_SOURCES=configcore.cpp configcore.h ngconfig.cpp ngconfig.h xmlconfig.cpp xmlconfig.h configio.h
libconfig_la_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libconfig_la_LIBADD= 
#$(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libmessage.la $(top_srcdir)/src/hed/libs/delegation/libdelegation.la $(top_srcdir)/src/hed/libs/infosys/libinfosys.la $(top_srcdir)/src/hed/libs/common/libarccommon.la ldif/libldif.la
libconfig_la_LDFLAGS=
