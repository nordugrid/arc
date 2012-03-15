ssmdir= $(pkgdatadir)/ssm

pkglibexec_SCRIPTS = ssm_master

ssm_DATA = SecureStompMessenger.py EncryptUtils.py message_db.py

EXTRA_DIST = $(ssm_DATA) $(pkglibexec_SCRIPTS) README.ssm ssm.cfg ssm.log.cfg

install-exec-hook:
	cd $(DESTDIR)$(pkglibexecdir) && \
        cp ssm_master ssm_master.orig && \
        sed "s,SSMDIR,\"$(ssmdir)\"," < ssm_master.orig > ssm_master && \
	rm -f ssm_master.orig
