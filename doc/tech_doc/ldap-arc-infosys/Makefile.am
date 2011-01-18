docdir = @docdir@

docsource = arc_infosys.tex *.png

doc_DATA = arc_infosys.pdf

STYLES = ng-logo.png IEEEtran.bst

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=)

$(STYLES):
	ln -s ../../templates/tex/$@

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc *.lof latex html

MAINTAINERCLEANFILES = $(doc_DATA)
