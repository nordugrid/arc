INPUT_FILES = finish-condor-job.in cancel-condor-job.in

libexec_SCRIPTS = submit-condor-job cancel-condor-job scan-condor-job finish-condor-job \
	configure-condor-env.sh condor_env.pm

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

EXTRA_DIST = $(INPUT_FILES) configure-fork-env.sh submit-condor-job scan-condor-job \
	configure-condor-env.sh condor_env.pm

CLEANFILES = finish-condor-job cancel-condor-job
