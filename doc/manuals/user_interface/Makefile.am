docdir = @docdir@

docsource = ui.tex

doc_DATA = ui.pdf

xrsl_PARTS = xrsl-urls.tex

STYLES = IEEEtran.bst color.sty framed.sty geometry.sty makeidx.sty gind.ist \
	ng-logo.png

$(doc_DATA): $(docsource) $(STYLES) $(xrsl_PARTS)
	pdflatex $(docsource) ; bibtex $(docsource) ; makeindex -s gind.ist $(docsource) ; pdflatex $(docsource) ; \
	pdflatex $(docsource)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ $@

$(xrsl_PARTS):
	$(LN_S) ../../tech_doc/xrsl/$@ $@

EXTRA_DIST = $(doc_DATA) $(STYLES) $(xrsl_PARTS) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES) $(xrsl_PARTS)
