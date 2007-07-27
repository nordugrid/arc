docdir = @docdir@

docsource = Doxyfile

doc_DATA = KnowARC-API.pdf

ngtest_PARTS = ngtest.tex
xrsl_PARTS = xrsl-urls.tex

KnowARC-API.pdf: $(docsource)
	doxygen && make -C latex && cp latex/refman.pdf KnowARC-API.pdf

EXTRA_DIST = $(docsource) hed KnowARC-API.pdf

CLEANFILES = latex

MAINTAINERCLEANFILES = $(doc_DATA)
