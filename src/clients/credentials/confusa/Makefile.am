if OAUTH_LIBRARY_ENABLED
CXX_FLAGS_WITH_OAUTH = $(OAUTH_CFLAGS)
LIBS_WITH_OAUTH = $(OAUTH_LIBS)
HEADERS_WITH_OAUTH = OAuthConsumer.h
SOURCES_WITH_OAUTH = OAuthConsumer.cpp

else
CXX_FLAGS_WITH_OAUTH = 
LIBS_WITH_OAUTH =
HEADERS_WITH_OAUTH =
SOURCES_WITH_OAUTH =
endif

lib_LTLIBRARIES = libconfusa.la
libconfusa_la_SOURCES = ConfusaCertHandler.cpp SAML2LoginClient.cpp ConfusaParserUtils.cpp idp/HakaClient.cpp idp/OpenIdpClient.cpp $(SOURCES_WITH_OAUTH) ConfusaCertHandler.h SAML2LoginClient.h ConfusaParserUtils.h idp/HakaClient.h idp/OpenIdpClient.h $(HEADERS_WITH_OAUTH)
libconfusa_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) $(OPENSSL_CFLAGS) $(CXX_FLAGS_WITH_OAUTH) -I$(top_srcdir)/include
libconfusa_la_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(LIBS_WITH_OAUTH)
libconfusa_la_LIBADD    = $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                  $(top_srcdir)/src/hed/libs/client/libarcclient.la \
                  $(top_srcdir)/src/hed/mcc/http/libmcchttp.la \
                  $(top_srcdir)/src/hed/libs/credential/libarccredential.la
         		           

