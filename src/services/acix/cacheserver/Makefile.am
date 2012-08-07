if SYSV_SCRIPTS_ENABLED
CACHESERVER_SCRIPT = acix-cache
else
CACHESERVER_SCRIPT =
endif
dist_initd_SCRIPTS = $(CACHESERVER_SCRIPT)

dist_sysconf_DATA = acix-cache.tac

SUBDIRS = $(TEST_DIR)
dist_SUBDIRS = test
