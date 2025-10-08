EXTRA_DIST = changelog control copyright rules watch source/format \
             $(srcdir)/*.install $(srcdir)/*.docs $(srcdir)/*.dirs \
             $(srcdir)/*.logrotate \
             $(srcdir)/*.postinst $(srcdir)/*.postrm $(srcdir)/*.prerm \
             $(srcdir)/*.links $(srcdir)/*.lintian-overrides \
             README.Debian README.source upstream/metadata

changelog: changelog.deb
	cp -p changelog.deb changelog

MAINTAINERCLEANFILES = changelog
