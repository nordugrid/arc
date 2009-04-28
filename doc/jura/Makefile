

.SUFFIXES: .pdf .tex

TARGETS= jura-tech-doc.pdf

.tex.pdf:
	pdflatex $<
	pdflatex $<
	

all:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.log *.aux

.PHONY: all clean
