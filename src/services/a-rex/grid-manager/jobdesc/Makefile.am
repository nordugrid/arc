JSDLDIR=jsdl
JSDLLIB=jsdl/libjsdl.la
JSDLSOURCES=job_jsdl.cpp
JSDLHEADERS=job_jsdl.h

if RSL_ENABLED
RSLDIR=rsl
RSLLIB=rsl/librsl.la
RSLSOURCES=job_xrsl.cpp xrsl.cpp
RSLHEADERS=job_xrsl.h xrsl.h
else
RSLDIR=
RSLLIB=
RSLSOURCES=
RSLHEADERS=
endif

SUBDIRS = $(JSDLDIR) $(RSLDIR)

noinst_LTLIBRARIES = libjobdesc.la

libjobdesc_la_SOURCES=$(RSLSOURCES) $(JSDLSOURCES) job.cpp jobdesc_util.cpp \
                      $(RSLHEADERS) $(JSDLHEADERS) job.h   jobdesc_util.h  
libjobdesc_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RSL_CFLAGS) -I$(top_srcdir)/include
libjobdesc_la_LDFLAGS=
libjobdesc_la_LIBADD=$(JSDLLIB) $(RSLLIB) $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                     $(top_srcdir)/src/hed/libs/job/libarcjob.la

