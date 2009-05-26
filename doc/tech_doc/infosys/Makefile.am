SUBDIRS=TechnicalHandbook

docdir = @docdir@

docsource = infosys_technical.tex

doc_DATA = infosys_technical.pdf

STYLES = IEEEtran.bst color.sty framed.sty geometry.sty makeidx.sty gind.ist \
	ng-logo.png grid.bib

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource) ; bibtex $(docsource) ; makeindex -s gind.ist $(docsource) ; pdflatex $(docsource) ; \
	pdflatex $(docsource)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ $@

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
