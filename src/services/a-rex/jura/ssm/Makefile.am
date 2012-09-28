ssmdir= $(pkgdatadir)/ssm

pkglibexec_SCRIPTS = ssm_master

ssm_DATA = ssm.py encrypt_utils.py message_db.py ssm.cfg ssm.log.cfg

EXTRA_DIST = $(ssm_DATA) $(pkglibexec_SCRIPTS) README.ssm

install-exec-hook:
	cd $(DESTDIR)$(pkglibexecdir) && \
        cp ssm_master ssm_master.orig && \
        sed "s,SSMDIR,\"$(ssmdir)\"," < ssm_master.orig > ssm_master && \
	rm -f ssm_master.orig
