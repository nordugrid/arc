sbin_SCRIPTS = nordugridmap

cronddir = @cronddir@
crond_DATA = nordugridmap.cron

man_MANS = nordugridmap.8

install-data-hook:
	cd $(DESTDIR)$(cronddir) && mv -f nordugridmap.cron nordugridmap

EXTRA_DIST = $(sbin_SCRIPTS) nordugridmap.conf
