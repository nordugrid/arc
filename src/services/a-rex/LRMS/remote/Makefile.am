INPUT_FILES = submit-remote-job.in cancel-remote-job.in scan-remote-job.in \
	log-functions.in remote-lrms-functions.in restore-remote-state.in \
	read-conf-value.pl.in downloader.wrapper.in uploader.wrapper.in 

libexec_SCRIPTS = submit-remote-job cancel-remote-job scan-remote-job \
	log-functions remote-lrms-functions restore-remote-state \
	read-conf-value.pl downloader.wrapper uploader.wrapper 

expand_paths = sed \
	-e 's,@libexecdir\@,$(libexecdir),g' \
	-e 's,@localsysconfdir\@,$(localsysconfdir),g' \
	-e 's,@sysconfdir\@,$(sysconfdir),g' \
	-e 's,@tmp_dir\@,$(tmp_dir),g' \
	-e 's,@nordugrid_location\@,$(nordugrid_location),g' \
	-e 's,@prefix\@,$(prefix),g'

# Create files from corresponding '.in' file
%: %.in Makefile
	rm -f $@ $@.tmp
	$(expand_paths) $< > $@.tmp
	chmod +x $@.tmp
	mv $@.tmp $@

EXTRA_DIST = $(INPUT_FILES)

CLEANFILES = $(libexec_SCRIPTS)
