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

libjobdesc_la_SOURCES=$(RSLSOURCES) $(JSDLSOURCES) job.cpp runtimeenvironment.cpp jobdesc_util.cpp \
                      $(RSLHEADERS) $(JSDLHEADERS) job.h   runtimeenvironment.h   jobdesc_util.h   error.h
#nodist_libjobdesc_la_SOURCES = jsdl_soapClientLib.cpp
libjobdesc_la_CXXFLAGS=$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_RSL_CFLAGS) -I$(top_srcdir)/include
libjobdesc_la_LDFLAGS=
libjobdesc_la_LIBADD=$(JSDLLIB) $(RSLLIB) $(top_srcdir)/src/hed/libs/common/libarccommon.la


#job_jsdl.cpp: jsdl_soapStub.h jsdl_soapH.h

#job_jsdl.cpp: jsdl_soapClientLib.cpp

#jsdl_soapClientLib.cpp jsdl_soapH.h jsdl_soapStub.h: jsdl_soap.h
#	$(SOAPCXX2) -n -p jsdl_soap jsdl_soap.h
#	@echo '#include <string>' > jsdl_soapStub.h.fixed
#	@cat jsdl_soapStub.h >> jsdl_soapStub.h.fixed
#	@rm jsdl_soapStub.h
#	@mv jsdl_soapStub.h.fixed jsdl_soapStub.h

#jsdl_soap.h: jsdl_complete.xsd jsdl_POSIXApplication.xsd jsdl_arc.xsd jsdl_typemap.dat
#	$(WSDL2H) -t jsdl_typemap.dat -o jsdl_soap.h jsdl_POSIXApplication.xsd jsdl_arc.xsd jsdl_complete.xsd

# CLEANFILES = jsdl_soap*
