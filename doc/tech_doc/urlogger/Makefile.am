docdir = @docdir@

docsource = urlogger.tex

doc_DATA = urlogger.pdf

STYLES = ng-logo.png

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=)

$(STYLES):
	ln -s ../templates/tex/$@

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc *.lof latex html ng-logo.png

MAINTAINERCLEANFILES = $(doc_DATA)
