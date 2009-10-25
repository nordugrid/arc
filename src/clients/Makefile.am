if XMLSEC_ENABLED
VOMS_SAML_CLIENT = saml
else
VOMS_SAML_CLIENT =
endif

SUBDIRS = data echo isis credentials arclib charon $(VOMS_SAML_CLIENT) srm jura wsrf
DIST_SUBDIRS = data echo isis credentials arclib charon saml srm jura wsrf

arcsysconfdir = $(sysconfdir)/@PACKAGE@
arcsysconf_DATA = client.conf
exampledir = $(pkgdatadir)/examples
example_DATA = client.conf.example
EXTRA_DIST = $(example_DATA) $(arcsysconf_DATA)
