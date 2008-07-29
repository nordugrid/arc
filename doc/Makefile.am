docdir = @docdir@

doc_DATA = ARC1-API.pdf ARC1-Services.pdf ARC1-ChainComponents.pdf

ngtest_PARTS = ngtest.tex
xrsl_PARTS = xrsl-urls.tex

ARC1-API.pdf: Doxyfile.api
	doxygen Doxyfile.api && make -C latex && cp latex/refman.pdf $@
	rm -rf latex

ARC1-Services.pdf: Doxyfile.services
	doxygen Doxyfile.services && make -C latex && cp latex/refman.pdf $@
	rm -rf latex

ARC1-ChainComponents.pdf: Doxyfile.chain
	doxygen Doxyfile.chain && make -C latex && cp latex/refman.pdf $@
	rm -rf latex


EXTRA_DIST = Doxyfile.api Doxyfile.services Doxyfile.chain hed $(doc_DATA)

CLEANFILES = latex

MAINTAINERCLEANFILES = $(doc_DATA)
