pkglib_LTLIBRARIES = libmcctls.la

libmcctls_la_SOURCES = PayloadTLSStream.cpp MCCTLS.cpp BIOMCC.cpp PayloadTLSMCC.cpp \
                       GlobusSigningPolicy.cpp DelegationSecAttr.cpp DelegationSH.cpp \
                       PayloadTLSStream.h   MCCTLS.h   BIOMCC.h   PayloadTLSMCC.h   \
                       GlobusSigningPolicy.h   DelegationSecAttr.h   DelegationSH.h
libmcctls_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) -I$(top_srcdir)/include
libmcctls_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                        $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                        $(top_srcdir)/src/hed/libs/credential/libarccredential.la \
                        $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(OPENSSL_LIBS) 
libmcctls_la_LDFLAGS  = -no-undefined -avoid-version -module
