SUBDIRS = tex_appendix tex_concept tex_future tex_installation tex_introduction tex_layout tex_maintenance tex_titlepage tex_usage images


docdir = @docdir@

docsource = Janitor.tex

docsources = literature.bib $(wildcard */*.tex)

doc_DATA = Janitor.pdf

all: $(doc_Data)

STYLES = IEEEtran.bst ng-logo.png

$(doc_DATA): $(docsource) $(docsources) $(STYLES)
	pdflatex $(docsource) ; bibtex $(docsource:.tex=) ; \
	makeindex -s gind.ist $(docsource) ; \
	pdflatex $(docsource) ; pdflatex $(docsource)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ .

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource) $(docsources)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
