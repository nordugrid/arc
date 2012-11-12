if GIIS_SERVICE_ENABLED
GIIS_SERVICE = giis
else
GIIS_SERVICE =
endif

if SYSV_SCRIPTS_ENABLED
GRID_INFOSYS_SCRIPT = nordugrid-arc-bdii nordugrid-arc-egiis nordugrid-arc-inforeg nordugrid-arc-slapd
else
GRID_INFOSYS_SCRIPT = 
endif

initd_SCRIPTS = $(GRID_INFOSYS_SCRIPT)

pkgdata_DATA = ConfigParser.pm

pkgdata_SCRIPTS = create-bdii-config create-inforeg-config create-slapd-config

SUBDIRS = $(GIIS_SERVICE)
DIST_SUBDIRS = giis

EXTRA_DIST = ConfigParser.pm
