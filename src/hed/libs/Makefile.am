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

# order is important!
SUBDIRS=common message credential loader data delegation security ws-addressing $(XMLSEC_DIR) ws-security wsrf client $(DBXML_DIR) infosys ws
DIST_SUBDIRS=common message credential loader data delegation security ws-addressing xmlsec ws-security wsrf client dbxml infosys ws
