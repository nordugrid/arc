docdir = @docdir@

docsource = ws-quick-guide.tex

doctitle = ws-quick-guide

doc_DATA = ws-quick-guide.pdf

STYLES = ng-logo.png

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource) ; bibtex $(doctitle) ; makeindex -s gind.ist $(doctitle) ; \
	pdflatex $(docsource) ; bibtex $(doctitle) \
	pdflatex $(docsource) ; \
	pdflatex $(docsource)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ $@


EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
