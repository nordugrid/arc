if WIN32
GRIDMAP =
else
GRIDMAP = gridmap
endif

if HED_ENABLED
BUILD_SOURCES = hed $(GRIDMAP)
else
BUILD_SOURCES = $(GRIDMAP)
endif

SUBDIRS = $(BUILD_SOURCES) configparser
DIST_SUBDIRS = hed gridmap configparser
