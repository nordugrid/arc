# Makefile of WS-Tutorial documentation

docdir = @docdir@

docsource = WS-tutorial.tex

doc_DATA = WS-tutorial.pdf

STYLES = ng-logo.png

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource:.tex=) ; bibtex $(docsource:.tex=) ; makeindex -s gind.ist $(docsource:.tex=) ; \
	pdflatex $(docsource:.tex=) ; bibtex $(docsource:.tex=) ; \
	pdflatex $(docsource:.tex=) \
	pdflatex $(docsource:.tex=)

$(STYLES):
	$(LN_S) ../../../templates/tex/$@ $@


EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
