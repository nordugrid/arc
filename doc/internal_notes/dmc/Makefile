

.SUFFIXES: .pdf .tex

TARGETS= dmc.pdf

.tex.pdf:
	pdflatex $<
	pdflatex $<
	

all:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.log *.aux

.PHONY: all clean
