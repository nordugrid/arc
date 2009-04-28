

.SUFFIXES: .pdf .tex

TARGETS= ARC1-Python-Developers-Guide.pdf

.tex.pdf:
	pdflatex $<
	pdflatex $<
	

all doc:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.log *.aux

.PHONY: all doc clean
