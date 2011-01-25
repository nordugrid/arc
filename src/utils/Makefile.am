if WIN32
GRIDMAP =
else
GRIDMAP = gridmap
endif

if HED_ENABLED
BUILD_SOURCES = hed $(GRIDMAP) testing
else
BUILD_SOURCES = $(GRIDMAP) testing
endif

SUBDIRS = $(BUILD_SOURCES)
DIST_SUBDIRS = hed gridmap testing

