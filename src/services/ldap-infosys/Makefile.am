if GIIS_SERVICE_ENABLED
GIIS_SERVICE = giis
else
GIIS_SERVICE =
endif

if SYSV_SCRIPTS_ENABLED
GRID_INFOSYS_SCRIPT = nordugrid-arc-bdii nordugrid-arc-egiis nordugrid-arc-inforeg nordugrid-arc-slapd nordugrid-arc-ldap-infosys
else
GRID_INFOSYS_SCRIPT = 
endif

initd_SCRIPTS = $(GRID_INFOSYS_SCRIPT)

if SYSTEMD_UNITS_ENABLED
GRID_INFOSYS_UNIT = nordugrid-arc-bdii.service nordugrid-arc-egiis.service nordugrid-arc-inforeg.service nordugrid-arc-slapd.service
else
GRID_INFOSYS_UNIT =
endif

units_DATA = $(GRID_INFOSYS_UNIT)

pkgdata_DATA = ConfigParser.pm

pkgdata_SCRIPTS = create-bdii-config create-inforeg-config create-slapd-config

SUBDIRS = $(GIIS_SERVICE)
DIST_SUBDIRS = giis

EXTRA_DIST = ConfigParser.pm nordugrid-arc-bdii.service nordugrid-arc-egiis.service nordugrid-arc-inforeg.service nordugrid-arc-slapd.service
