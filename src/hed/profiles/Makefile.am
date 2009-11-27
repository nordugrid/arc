
# Get all files with name like:  <profilename>/<profilename>.xml
profile_DATA := $(wildcard $(foreach dir,$(wildcard *),$(dir)/$(dir).xml))
profiledir = $(pkgdatadir)/profiles

# All files in the directories where there is a profile (with correct naming)
# Filter out the profiles from the profile example directory
profileexample_DATA := $(filter-out profile_DATA,$(foreach dir,$(dir $(profile_DATA)),$(wildcard $(dir)*)))

profileexampledir = $(pkgdatadir)/examples/config

EXTRA_DIST = $(profile_DATA) $(profileexamples_DATA)

# Modified version of install-profileexampleDATA which preserve subdirectory
install-profileexampleDATA: $(profileexample_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(profileexampledir)" || $(MKDIR_P) "$(DESTDIR)$(profileexampledir)"
	@list='$(profileexample_DATA)'; test -n "$(profileexampledir)" || list=; \
        for p in $$list; do \
          if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
          echo "$$d$$p"; \
        done | $(am__base_list) | \
        while read files; do \
          for f in $$files ; do \
            subdir=`dirname $$f` ;\
	    $(MKDIR_P) "$(DESTDIR)$(profileexampledir)/$$subdir" ;\
            echo " $(INSTALL_DATA) $$f '$(DESTDIR)$(profileexampledir)/$$subdir'"; \
            $(INSTALL_DATA) $$f "$(DESTDIR)$(profileexampledir)/$$subdir" || exit $$?; \
          done ; \
        done
