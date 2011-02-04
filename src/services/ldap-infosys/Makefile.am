initddir=@initddir@

if GIIS_SERVICE_ENABLED
GIIS_SERVICE = giis
else
GIIS_SERVICE =
endif


if SYSV_SCRIPTS_ENABLED
GRID_INFOSYS_SCRIPT = grid-infosys
else
GRID_INFOSYS_SCRIPT =
endif

initd_SCRIPTS = $(GRID_INFOSYS_SCRIPT)

pkglib_DATA = grid-infosys-bdii4 grid-infosys-bdii5 ConfigParser.pm

SUBDIRS=$(GIIS_SERVICE)
DIST_SUBDIRS=giis

EXTRA_DIST = ConfigParser.pm
