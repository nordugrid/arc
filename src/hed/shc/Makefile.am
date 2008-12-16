if XMLSEC_ENABLED
LIBS_WITH_XMLSEC = x509tokensh/libx509tokensh.la saml2sso_assertionconsumersh/libsaml2ssoassertionconsumersh.la
SUBDIRS_XMLSEC= x509tokensh saml2sso_assertionconsumersh
else
LIBS_WITH_XMLSEC = 
SUBDIRS_XMLSEC =
endif
SUBDIRS = allowpdp denypdp countpdp simplelistpdp arcpdp pdpserviceinvoker arcauthzsh delegationsh usernametokensh gaclpdp $(SUBDIRS_XMLSEC)
DIST_SUBDIRS = allowpdp denypdp countpdp simplelistpdp arcpdp pdpserviceinvoker arcauthzsh delegationsh usernametokensh gaclpdp x509tokensh saml2sso_assertionconsumersh
noinst_PROGRAMS = test testinterface
# classload_test 

pkglib_LTLIBRARIES = libarcpdc.la
libarcpdc_la_SOURCES  = SecHandlerPlugin.cpp
libarcpdc_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcpdc_la_LIBADD   = allowpdp/liballowpdp.la denypdp/libdenypdp.la countpdp/libcountpdp.la \
                        simplelistpdp/libsimplelistpdp.la arcpdp/libarcpdp.la gaclpdp/libgaclpdp.la \
                        pdpserviceinvoker/libarcpdpserviceinvoker.la delegationsh/libdelegationsh.la \
                        arcauthzsh/libarcauthzsh.la usernametokensh/libusernametokensh.la \
                        $(LIBS_WITH_XMLSEC) \
                        $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                        $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                        $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                        $(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
                        $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                        $(LIBXML2_LIBS)
libarcpdc_la_LDFLAGS  = -no-undefined -avoid-version -module

test_SOURCES  = test.cpp
test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
test_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
test_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

testinterface_SOURCES  = testinterface.cpp
testinterface_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
testinterface_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
testinterface_LDADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la


#classload_test_SOURCES = classload_test.cpp
#classload_test_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
#classload_test_LDFLAGS = $(GLIBMM_LIBS) $(LIBXML2_LIBS)
#classload_test_LDADD = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/security/libarcsecurity.la

