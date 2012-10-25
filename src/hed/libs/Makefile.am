if DBXML_ENABLED
DBXML_DIR = dbxml
else
DBXML_DIR =
endif

if XMLSEC_ENABLED
XMLSEC_DIR = xmlsec
else
XMLSEC_DIR =
endif

if GLOBUSUTILS_ENABLED
GLOBUSUTILS_DIR = globusutils
else
GLOBUSUTILS_DIR =
endif

# order is important!
SUBDIRS = common loader message crypto cryptomod \
	credential credentialmod data delegation security \
	ws-addressing $(XMLSEC_DIR) ws-security wsrf communication \
	client $(DBXML_DIR) infosys ws $(GLOBUSUTILS_DIR) \
	credentialstore
DIST_SUBDIRS = common loader message crypto cryptomod \
	credential credentialmod data delegation security \
	ws-addressing xmlsec ws-security wsrf communication \
	client dbxml infosys ws globusutils credentialstore
