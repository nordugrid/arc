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
	credential credentialmod data security \
	ws-addressing $(XMLSEC_DIR) ws-security wsrf delegation \
	communication compute $(DBXML_DIR) ws infosys $(GLOBUSUTILS_DIR) \
	credentialstore
DIST_SUBDIRS = common loader message crypto cryptomod \
	credential credentialmod data security \
	ws-addressing xmlsec ws-security wsrf delegation \
	communication compute dbxml ws infosys globusutils credentialstore
