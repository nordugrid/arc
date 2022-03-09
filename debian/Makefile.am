EXTRA_DIST = changelog compat control copyright rules watch source/format \
             $(srcdir)/*.install $(srcdir)/*.docs $(srcdir)/*.dirs \
             $(srcdir)/*.enable $(srcdir)/*.no-enable $(srcdir)/*.logrotate \
             $(srcdir)/*.postinst $(srcdir)/*.postrm $(srcdir)/*.prerm \
             $(srcdir)/*.links $(srcdir)/*.lintian-overrides \
             README.Debian README.source

changelog: changelog.deb
	cp -p changelog.deb changelog

MAINTAINERCLEANFILES = changelog
