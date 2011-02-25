if WIN32
GRIDMAP =
ARCCONFCONVERT =
else
GRIDMAP = gridmap
ARCCONFCONVERT = arcconfconvert
endif

bin_SCRIPTS = ARCCONFCONVERT

man_MANS = arcconfconvert.1

if HED_ENABLED
BUILD_SOURCES = hed $(GRIDMAP) testing
else
BUILD_SOURCES = $(GRIDMAP) testing
endif

SUBDIRS = $(BUILD_SOURCES)
DIST_SUBDIRS = hed gridmap testing

