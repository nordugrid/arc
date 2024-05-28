if DATA_CLIENT_ENABLED
DATA_CLIENT = data
else
DATA_CLIENT =
endif

if CREDENTIALS_CLIENT_ENABLED
CREDENTIALS_CLIENT = credentials
else
CREDENTIALS_CLIENT =
endif

if COMPUTE_CLIENT_ENABLED
COMPUTE_CLIENT = compute
else
COMPUTE_CLIENT =
endif

SUBDIRS = $(DATA_CLIENT) $(CREDENTIALS_CLIENT) $(COMPUTE_CLIENT) pyarcrest
DIST_SUBDIRS = data credentials compute pyarcrest

arcsysconfdir = $(sysconfdir)/arc
arcsysconf_DATA = client.conf

exampledir = $(pkgdatadir)/examples
example_DATA = client.conf

BASH_COMPLETION_SOURCE = client.bash_completion

arc-client-tools: $(BASH_COMPLETION_SOURCE)
	cp $< $@

bashcompletiondir = $(bashcompdir)
bashcompletion_DATA = arc-client-tools

EXTRA_DIST = $(example_DATA) $(arcsysconf_DATA) $(BASH_COMPLETION_SOURCE)

CLEANFILES = arc-client-tools
