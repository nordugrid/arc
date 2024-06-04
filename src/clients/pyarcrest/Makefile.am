install:
	PIP_OPTIONS="--no-deps --disable-pip-version-check --verbose" ; \
	if $(PYTHON) -m pip install --help | grep -q -- --use-pep517 ; then \
	    PIP_OPTIONS="$$PIP_OPTIONS --use-pep517" ; \
	fi ; \
	if $(PYTHON) -m pip install --help | grep -q -- --no-build-isolation ; then \
	    PIP_OPTIONS="$$PIP_OPTIONS --no-build-isolation" ; \
	fi ; \
	echo PIP options: $$PIP_OPTIONS ; \
	$(PYTHON) -m pip install $$PIP_OPTIONS \
	    --root $(DESTDIR) --prefix $(prefix) $(srcdir)

EXTRA_DIST = \
	README.md \
	setup.cfg \
	setup.py \
	src/pyarcrest/arc.py \
	src/pyarcrest/cli/__init__.py \
	src/pyarcrest/cli/arcrest.py \
	src/pyarcrest/errors.py \
	src/pyarcrest/__init__.py \
	src/pyarcrest/http.py \
	src/pyarcrest/x509.py
