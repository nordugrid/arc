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

if COMPUTE_CLIENT_ENABLED
COMPUTE_CLIENT = compute
else
COMPUTE_CLIENT =
endif

if SAML_CLIENT_ENABLED
SAML_CLIENT = saml
else
SAML_CLIENT =
endif

SUBDIRS = $(DATA_CLIENT) $(ECHO_CLIENT) $(CREDENTIALS_CLIENT) $(COMPUTE_CLIENT) $(SAML_CLIENT)
DIST_SUBDIRS = data echo credentials compute saml

arcsysconfdir = $(sysconfdir)/arc
arcsysconf_DATA = client.conf
exampledir = $(pkgdatadir)/examples
example_DATA = client.conf
EXTRA_DIST = $(example_DATA) $(arcsysconf_DATA)

