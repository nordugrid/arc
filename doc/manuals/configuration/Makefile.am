docdir = @docdir@

docsource = configuration_manual.tex

doc_DATA = configuration_manual.pdf

STYLES = ng-logo.png grid.bib
SCHEMAS = src/hed/daemon/schema/base.xsd \
					src/hed/libs/loader/schema/loader.xsd \
					src/hed/mcc/tcp/schema/tcp.xsd \
					src/hed/mcc/tls/schema/tls.xsd

$(doc_DATA): $(docsource) $(STYLES) $(notdir $(SCHEMAS))
	pdflatex $(docsource:.tex=) ; bibtex $(docsource:.tex=) ; makeindex -s gind.ist $(docsource:.tex=) ; \
	pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ $@

$(notdir $(SCHEMAS)):
	$(LN_S) $(top_srcdir)/../$(filter %/$@, $(SCHEMAS)) $@

EXTRA_DIST = $(doc_DATA) $(STYLES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES) *.xsd
