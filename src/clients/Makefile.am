if DATA_CLIENT_ENABLED
DATA_CLIENT = data
else
DATA_CLIENT =
endif

if ECHO_CLIENT_ENABLED
ECHO_CLIENT = echo
else
ECHO_CLIENT =
endif

if CREDENTIALS_CLIENT_ENABLED
CREDENTIALS_CLIENT = credentials
else
CREDENTIALS_CLIENT =
endif

if ARCLIB_CLIENT_ENABLED
ARCLIB_CLIENT = arclib
else
ARCLIB_CLIENT =
endif

if SAML_CLIENT_ENABLED
SAML_CLIENT = saml
else
SAML_CLIENT =
endif

if JURA_CLIENT_ENABLED
JURA_CLIENT = jura
else
JURA_CLIENT =
endif

if WSRF_CLIENT_ENABLED
WSRF_CLIENT = wsrf
else
WSRF_CLIENT =
endif

if CACHE_CLIENT_ENABLED
CACHE_CLIENT = cache
else
CACHE_CLIENT =
endif

SUBDIRS = $(DATA_CLIENT) $(ECHO_CLIENT) $(CREDENTIALS_CLIENT) $(ARCLIB_CLIENT) $(SAML_CLIENT) $(JURA_CLIENT) $(WSRF_CLIENT) $(CACHE_CLIENT)
DIST_SUBDIRS = data echo credentials arclib saml jura wsrf cache

arcsysconfdir = $(sysconfdir)/arc
arcsysconf_DATA = client.conf
exampledir = $(pkgdatadir)/examples
example_DATA = client.conf
EXTRA_DIST = $(example_DATA) $(arcsysconf_DATA)

