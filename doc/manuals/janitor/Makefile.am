# Makefile of Janitor documentation

docdir = @docdir@

docsource = Janitor.tex

doc_DATA = Janitor.pdf

images     = images/dependencies.dot images/dependencies.tex \
           images/arex-stages.svg images/arex-stages.pdf \
           images/janitor_integration_2nd_edition.svg images/janitor_integration_2nd_edition.pdf \
           images/janitor_integration.svg images/janitor_integration.pdf \
           images/protege_JASPAR.png images/RE_states.png \
           images/tar_folder.pdf images/tar_folder.svg \
           images/WS_structure.pdf

docsrcdeps = $(images) \
             tex_appendix/appendix.tex tex_concept/concept.tex tex_future/future.tex \
             tex_installation/installation.tex tex_introduction/introduction.tex tex_layout/layout.tex \
             tex_maintenance/maintenance.tex tex_titlepage/titlepage.tex tex_usage/usage.tex

images/dependencies.tex: images/dependencies.dot
	dot2tex --figonly $< -o $@

JANITOR_SOURCECODE = $(top_srcdir)/src/services/janitor/module

STYLES = IEEEtran.bst color.sty framed.sty geometry.sty makeidx.sty gind.ist \
        ng-logo.png

$(doc_DATA): $(docsource) $(STYLES) $(docsrcdeps)
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

EXTRA_DIST = $(doc_DATA) $(docsource) $(docsrcdeps)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc  $(STYLES)

MAINTAINERCLEANFILES = $(doc_DATA) images/dependencies.tex


