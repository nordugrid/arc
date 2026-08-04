# Set PYTHON variables from the alternative python

PYTHON = @ALTPYTHON@
PYTHON_VERSION = @ALTPYTHON_VERSION@

include $(top_srcdir)/src/clients/pyarcrest/install.mk

all-local:
	rm -rf src setup.py setup.cfg; \
	cp -R $(top_srcdir)/src/clients/pyarcrest/src src; \
	cp -a $(top_srcdir)/src/clients/pyarcrest/setup.py . ; \
	sed '/pyarcrest.cli.arcrest:main/s/arcrest /arcrest-$(PYTHON_VERSION) /' $(top_srcdir)/src/clients/pyarcrest/setup.cfg > setup.cfg

clean-local:
	rm -rf build src setup.py setup.cfg
