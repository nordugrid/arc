SUBDIRS = tex_appendix tex_concept tex_future tex_installation tex_introduction tex_layout tex_maintenance tex_titlepage tex_usage images

docdir = @docdir@

docsource = Janitor.tex

doc_DATA = Janitor.pdf

STYLES = IEEEtran.bst ng-logo.png

$(doc_DATA): $(docsource) $(STYLES)
	pdflatex $(docsource) ; bibtex $(docsource) ; makeindex -s gind.ist $(docsource) ; pdflatex $(docsource) ; \
	pdflatex $(docsource)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ .

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
