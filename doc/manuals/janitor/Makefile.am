# Makefile of Janitor documentation

docdir = @docdir@

docsource = Janitor.tex

doc_DATA = Janitor.pdf

JANITOR_SOURCECODE = $(top_srcdir)/src/services/janitor/module

STYLES = IEEEtran.bst color.sty framed.sty geometry.sty makeidx.sty gind.ist \
        ng-logo.png

$(doc_DATA): $(docsource) $(STYLES) manperl
	pdflatex $(docsource) ; bibtex $(docsource) ; makeindex -s gind.ist $(docsource) ; pdflatex $(docsource) ; \
	pdflatex $(docsource)

$(STYLES):
		$(LN_S) ../../templates/tex/$@ $@

manperl:
	@[ -d manperl ] || mkdir manperl
	for i in $(JANITOR_SOURCECODE)/*; do \
		echo $$i;\
		if grep -q head1 $$i; then \
			dest="manperl/Janitor::$$(basename $$i .pm).3pm" ; \
			if [ ! -e $$dest ] || [ $$i -nt $$dest ] ; then \
				pod2man $$i > $$dest; \
			else \
				echo skipping $$i ... - docs still up to date; \
			fi \
		fi \
	done
	for i in $(JANITOR_SOURCECODE)/Catalog/*; do \
		echo $$i;\
		if grep -q head1 $$i; then \
			dest="manperl/Catalog/Janitor::$$(basename $$i .pm).3pm" ; \
			if [ ! -e $$dest ] || [ $$i -nt $$dest ] ; then \
				pod2man $$i > $$dest; \
			else \
				echo skipping $$i ... - docs still up to date; \
			fi \
		fi \
	done

EXTRA_DIST = $(doc_DATA) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc  $(STYLES)

MAINTAINERCLEANFILES = $(doc_DATA)


