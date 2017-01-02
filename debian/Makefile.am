EXTRA_DIST = changelog compat control copyright rules watch source/format \
             $(srcdir)/*.install $(srcdir)/*.docs $(srcdir)/*.dirs \
             $(srcdir)/*.default $(srcdir)/*.logrotate \
             $(srcdir)/*.postinst $(srcdir)/*.postrm \
             $(srcdir)/*.lintian-overrides README.Debian README.source \
             nordugrid-arc-arex.install.in nordugrid-arc-arex.install.extra

changelog: changelog.deb
	cp -p changelog.deb changelog

if PYTHON_LRMS_ENABLED
AREX_FILELISTS = $(srcdir)/nordugrid-arc-arex.install.in $(srcdir)/nordugrid-arc-arex.install.extra
else
AREX_FILELISTS = $(srcdir)/nordugrid-arc-arex.install.in
endif

all-am: nordugrid-arc-arex.install

nordugrid-arc-arex.install: $(AREX_FILELISTS)
	cat $(AREX_FILELISTS) > $@

CLEANFILES = nordugrid-arc-arex.install

dist-hook:
	 rm -rf $(distdir)/debian/nordugrid-arc-arex.install

MAINTAINERCLEANFILES = changelog
