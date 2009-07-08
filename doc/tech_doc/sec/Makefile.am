docdir = @docdir@

docsource = SecurityFrameworkofARC1.tex

doc_DATA = SecurityFrameworkofARC1.pdf

IMAGES = Secpaper-img1.png \
         SecurityFrameworkofARC1-img1.jpg SecurityFrameworkofARC1-img2.jpg \
         SecurityFrameworkofARC1-img3.png SecurityFrameworkofARC1-img4.png \
         SecurityFrameworkofARC1-img5.png SecurityFrameworkofARC1-img6.png \
         SecurityFrameworkofARC1-img7.png SecurityFrameworkofARC1-img8.png

$(doc_DATA): $(docsource) $(IMAGES)
	pdflatex $(docsource) ; pdflatex $(docsource)

EXTRA_DIST = $(doc_DATA) $(IMAGES) $(docsource)

CLEANFILES = *.blg *.bbl *.ind *.ilg *.out *.log *.idx *.aux *.toc

MAINTAINERCLEANFILES = $(doc_DATA)
