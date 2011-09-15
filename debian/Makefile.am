EXTRA_DIST = changelog compat control copyright rules source/format \
             $(srcdir)/*.doc-base* $(srcdir)/*.install $(srcdir)/*.docs \
             $(srcdir)/*.default $(srcdir)/*.logrotate \
             $(srcdir)/*.lintian-overrides README.Debian README.source

changelog: changelog.deb
	cp -p changelog.deb changelog

MAINTAINERCLEANFILES = changelog
