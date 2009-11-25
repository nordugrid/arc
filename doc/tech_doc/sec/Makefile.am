docdir = @docdir@

docsource = arc-security-documentation.tex

doc_DATA = arc-security-documentation.pdf

STYLES = IEEEtran.bst ng-logo.png grid.bib

IMAGES = figures/*.pdf figures/*.png

$(doc_DATA): $(docsource) $(IMAGES)
	pdflatex $(docsource:.tex=) ; bibtex $(docsource:.tex=) ; pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ .

EXTRA_DIST = $(doc_DATA) $(IMAGES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
