if SYSV_SCRIPTS_ENABLED
INDEXSERVER_SCRIPT = acix-index
else
INDEXSERVER_SCRIPT =
endif
dist_initd_SCRIPTS = $(INDEXSERVER_SCRIPT)

dist_sysconf_DATA = acix-index.tac

SUBDIRS = $(TEST_DIR)
dist_SUBDIRS = test
