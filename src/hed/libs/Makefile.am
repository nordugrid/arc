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
SUBDIRS = common loader message credential data delegation security \
	ws-addressing $(XMLSEC_DIR) ws-security wsrf client $(DBXML_DIR) \
	infosys ws $(GLOBUSUTILS_DIR) job
DIST_SUBDIRS = common loader message credential data delegation security \
	ws-addressing xmlsec ws-security wsrf client dbxml \
	infosys ws globusutils job
