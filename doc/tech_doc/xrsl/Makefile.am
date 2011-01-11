docdir = @docdir@

docsource = *.tex

doc_DATA = xrsl.pdf

STYLES = IEEEtran.bst color.sty framed.sty geometry.sty makeidx.sty gind.ist grid.bib ng-logo.png

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

%.pdf: %.tex $(STYLES)
	pdflatex $* ; bibtex $* ; makeindex -s gind.ist $* ; pdflatex $* ; pdflatex $*

$(STYLES):
	ln -s ../templates/tex/$@

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc latex html

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
