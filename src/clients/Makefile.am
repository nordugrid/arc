if XMLSEC_ENABLED
VOMS_SAML_CLIENT = voms
else
VOMS_SAML_CLIENT =
endif

SUBDIRS = data echo wsrf isis credentials arclib charon $(VOMS_SAML_CLIENT) srm lutsclient
DIST_SUBDIRS = data echo wsrf isis credentials arclib charon voms srm lutsclient
