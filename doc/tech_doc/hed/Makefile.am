docdir = @docdir@

docsource = ARCHED_article.tex

doc_DATA = ARCHED_article.pdf

STYLES = IEEEtran.bst color.sty framed.sty geometry.sty makeidx.sty gind.ist \
	ng-logo.png grid.bib nordugrid.bib

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource:.tex=) ; bibtex $(docsource:.tex=) ; makeindex -s gind.ist $(docsource:.tex=) ; \
	pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ $@

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
