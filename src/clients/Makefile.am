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

if ISIS_CLIENT_ENABLED
ISIS_CLIENT = isis
else
ISIS_CLIENT =
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

if CHARON_CLIENT_ENABLED
CHARON_CLIENT = charon
else
CHARON_CLIENT =
endif

if SAML_CLIENT_ENABLED
SAML_CLIENT = saml
else
SAML_CLIENT =
endif

if SRM_CLIENT_ENABLED
SRM_CLIENT = srm
else
SRM_CLIENT =
endif

if JURA_CLIENT_ENABLED
JURA_CLIENT = jura
else
JURA_CLIENT =
endif

SUBDIRS = $(DATA_CLEINT) $(ECHO_CLIENT) $(ISIS_CLIENT) $(CREDENTIALS_CLIENT) $(ARCLIB_CLIENT) $(CHARON_CLIENT) $(SAML_CLIENT) $(SRM_CLIENT) $(JURA_CLIENT) wsrf
DIST_SUBDIRS = data echo isis credentials arclib charon saml srm jura wsrf

arcsysconfdir = $(sysconfdir)/@PACKAGE@
arcsysconf_DATA = client.conf
exampledir = $(pkgdatadir)/examples
example_DATA = client.conf.example
EXTRA_DIST = $(example_DATA) $(arcsysconf_DATA)





