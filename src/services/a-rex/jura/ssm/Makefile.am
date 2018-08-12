ssmdir= $(pkgdatadir)/ssm

pkglibexec_SCRIPTS = ssmsend

ssm_DATA = ssm2.py crypto.py brokers.py sender.cfg __init__.py

EXTRA_DIST = $(ssm_DATA) README.ssm

install-exec-hook:
	cd $(DESTDIR)$(pkglibexecdir) && \
        cp ssmsend ssmsend.orig && \
        sed "s,SSMDIR,\"$(ssmdir)\"," < ssmsend.orig > ssmsend && \
	rm -f ssmsend.orig
