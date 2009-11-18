docdir = @docdir@

docsource = arc-security-documentation.tex

doc_DATA = arc-security-documentation.pdf

STYLES = IEEEtran.bst ng-logo.png grid.bib

IMAGES = Secpaper-img1.png \
         SecurityFrameworkofARC1-img1.jpg SecurityFrameworkofARC1-img2.jpg \
         SecurityFrameworkofARC1-img3.png SecurityFrameworkofARC1-img4.png \
         SecurityFrameworkofARC1-img5.png SecurityFrameworkofARC1-img6.png \
         SecurityFrameworkofARC1-img7.png SecurityFrameworkofARC1-img8.png

$(doc_DATA): $(docsource) $(IMAGES)
	pdflatex $(docsource:.tex=) ; bibtex $(docsource:.tex=) ; pdflatex $(docsource:.tex=) ; pdflatex $(docsource:.tex=)

$(STYLES):
	$(LN_S) ../../templates/tex/$@ .

EXTRA_DIST = $(doc_DATA) $(IMAGES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA) $(STYLES)
