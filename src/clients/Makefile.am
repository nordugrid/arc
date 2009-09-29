if XMLSEC_ENABLED
VOMS_SAML_CLIENT = saml
else
VOMS_SAML_CLIENT =
endif

SUBDIRS = data echo isis credentials arclib charon $(VOMS_SAML_CLIENT) srm jura wsrf
DIST_SUBDIRS = data echo isis credentials arclib charon saml srm jura wsrf

arcsysconfdir = $(sysconfdir)/arc
arcsysconf_DATA = client.conf
doc_DATA = client.conf.example
EXTRA_DIST = $(doc_DATA) $(arcsysconf_DATA)
