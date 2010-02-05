docdir = @docdir@

docsource = configuration_manual.tex

doc_DATA = configuration_manual.pdf

STYLES = ng-logo.png grid.bib
SCHEMAS = src/hed/daemon/schema/base.xsd \
	src/hed/libs/loader/schema/loader.xsd \
	src/hed/mcc/tcp/schema/tcp.xsd \
	src/hed/mcc/tls/schema/tls.xsd \
	src/hed/mcc/gsi/schema/gsi.xsd \
	src/hed/mcc/http/schema/http.xsd \
	src/hed/shc/arcauthzsh/schema/ArcAuthZ.xsd \
	src/hed/shc/usernametokensh/schema/UsernameTokenSH.xsd \
	src/hed/shc/x509tokensh/schema/X509TokenSH.xsd \
	src/hed/shc/arcpdp/schema/ArcPDP.xsd \
	src/hed/shc/xacmlpdp/schema/XACMLPDP.xsd \
	src/hed/shc/delegationpdp/schema/DelegationPDP.xsd \
	src/hed/shc/pdpserviceinvoker/schema/PDPServiceInvoker.xsd \
	src/hed/shc/simplelistpdp/schema/SimpleListPDP.xsd

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
