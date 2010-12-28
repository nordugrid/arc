docdir = @docdir@

docsource = monitor.tex\
           attlist.png jobstat2.png loadmon.png match1.png match2.png\
           match3.png quelist.png storage.png userlist.png clusdes.jpg\
           jobstat1.jpg vo-users.jpg

doc_DATA = monitor.pdf

STYLES = grid.bib nordugrid.bib ng-logo.png

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource:.tex=) ; bibtex $(docsource:.tex=) ; makeindex -s gind.ist $(docsource:.tex=) ; \
	pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=)

$(STYLES):
	ln -s ../templates/tex/$@

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc *.lof latex html

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
