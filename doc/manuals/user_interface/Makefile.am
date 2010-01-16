docdir = @docdir@

doctitle=ui

docsource = $(doctitle).tex

doc_DATA = $(doctitle).pdf

xrsl_PARTS = xrsl-urls.tex

chelonia_PARTS = chelonia-cli.tex

STYLES = grid.bib nordugrid.bib ng-logo.png

$(doc_DATA): $(docsource) $(STYLES) $(xrsl_PARTS) $(chelonia_PARTS)
	pdflatex $(docsource) ; bibtex $(doctitle) ; makeindex -s gind.ist $(doctitle) ; pdflatex $(docsource) ; \
	pdflatex $(docsource)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ $@

$(xrsl_PARTS):
	$(LN_S) ../../tech_doc/xrsl/$@ $@

$(chelonia_PARTS):
	$(LN_S) ../chelonia/$@ $@

EXTRA_DIST = $(doc_DATA) $(STYLES) $(xrsl_PARTS) $(chelonia_PARTS) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES) $(xrsl_PARTS) $(chelonia_PARTS)
