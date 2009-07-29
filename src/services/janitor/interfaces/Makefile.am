# Make file for all interfaces (./interfaces)

STANDARD = commandline


# Compile and install interfaces/webservice
#                 and interfaces/client
if JANITOR_WEBSERVICE_ENABLED
# PERL_CFLAGS, PERL_LDFLAGS, ENABLE_COMPILE
# "webservice" lassen wir heute mal weg
EXTRA = webservice client
else
EXTRA =
endif


SUBDIRS = $(STANDARD) $(EXTRA)