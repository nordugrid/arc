if SYSV_SCRIPTS_ENABLED
GRID_INFOSYS_SCRIPT = nordugrid-arc-aris
else
GRID_INFOSYS_SCRIPT = 
endif

initd_SCRIPTS = $(GRID_INFOSYS_SCRIPT)

if SYSTEMD_UNITS_ENABLED
GRID_INFOSYS_UNIT = nordugrid-arc-aris.service nordugrid-arc-slapd.service
else
GRID_INFOSYS_UNIT =
endif

units_DATA = $(GRID_INFOSYS_UNIT)

pkgdata_SCRIPTS = create-bdii-config create-slapd-config

EXTRA_DIST = nordugrid-arc-aris.service nordugrid-arc-slapd.service
