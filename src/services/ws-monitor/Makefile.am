SUBDIRS = man mon-icons lang includes

monitordir    = @ws_monitor_prefix@
monitor_DATA = $(srcdir)/*.php $(srcdir)/*.js README

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(monitordir)/cache

EXTRA_DIST = $(monitor_DATA)
