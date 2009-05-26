# Makefile of WS-Tutorial documentation

docdir = @docdir@

docsource = WS-tutorial.tex

doc_DATA = WS-tutorial.pdf

STYLES = IEEEtran.bst color.sty framed.sty geometry.sty makeidx.sty gind.ist \
	ng-logo.png

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource) ; bibtex $(docsource) ; makeindex -s gind.ist $(docsource) ; pdflatex $(docsource) ; \
	pdflatex $(docsource)

$(STYLES):
	$(LN_S) ../../../templates/tex/$@ $@


EXTRA_DIST = $(doc_DATA) # ADD EXAMPLES DIRECTORY HERE

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
