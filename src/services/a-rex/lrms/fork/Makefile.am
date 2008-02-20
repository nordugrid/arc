INPUT_FILES = submit-fork-job.in cancel-fork-job.in scan-fork-job.in

libexec_SCRIPTS = submit-fork-job cancel-fork-job scan-fork-job \
	configure-fork-env.sh

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

EXTRA_DIST = $(INPUT_FILES) configure-fork-env.sh

CLEANFILES = submit-fork-job cancel-fork-job scan-fork-job
