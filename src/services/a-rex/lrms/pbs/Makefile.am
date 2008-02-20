INPUT_FILES = submit-pbs-job.in cancel-pbs-job.in scan-pbs-job.in

libexec_SCRIPTS = submit-pbs-job cancel-pbs-job scan-pbs-job \
	configure-pbs-env.sh

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

EXTRA_DIST = $(INPUT_FILES) configure-pbs-env.sh

CLEANFILES = submit-pbs-job cancel-pbs-job scan-pbs-job
