dist_pkgdata_DATA = configure-DGBridge-env.sh
pkgdata_SCRIPTS = DGAuthplug.py DGBridgeDataPlugin.py DGLog2XML.py \
                  scan-DGBridge-job submit-DGBridge-job cancel-DGBridge-job

profiledir = $(pkgdatadir)/profiles
profile_DATA = dgbridge.xml

install-data-hook:
	cp -p $(DESTDIR)$(profiledir)/dgbridge.xml $(DESTDIR)$(profiledir)/dgbridge.xml.orig && \
        sed -e "s,/usr/libexec/arc,${pkgdatadir},g" -e "s,/usr/lib64/arc,${pkglibdir},g" \
               < $(DESTDIR)$(profiledir)/dgbridge.xml.orig > $(DESTDIR)$(profiledir)/dgbridge.xml ;\
        rm -f $(DESTDIR)$(profiledir)/dgbridge.xml.orig

EXTRA_DIST = README.DGBridge dgbridge_service.ini dgbridge.conf $(profile_DATA)

