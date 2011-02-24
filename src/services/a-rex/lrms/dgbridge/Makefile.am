dist_pkglib_DATA = configure-DGBridge-env.sh
dist_pkglibexec_SCRIPTS = scan-DGBridge-job submit-DGBridge-job cancel-DGBridge-job \
                          DGAuthplug.py DGBridgeDataPlugin.py DGLog2XML.py

profiledir = $(pkgdatadir)/profiles
profile_DATA = dgbridge.xml

install-data-hook:
	cp -p $(DESTDIR)$(profiledir)/dgbridge.xml $(DESTDIR)$(profiledir)/dgbridge.xml.orig && \
        sed "s,/usr/libexec/arc,${pkglibexecdir},g" < $(DESTDIR)$(profiledir)/dgbridge.xml.orig | \
        sed "s,/usr/lib64/arc,${pkglibdir},g" > $(DESTDIR)$(profiledir)/dgbridge.xml \
        rm -f $(DESTDIR)$(profiledir)/dgbridge.xml.orig

EXTRA_DIST = README.DGBridge dgbridge_service.ini dgbridge.conf $(profile_DATA)

