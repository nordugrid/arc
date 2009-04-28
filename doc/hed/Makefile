

.SUFFIXES: .pdf .tex

TARGETS= counter.pdf

.tex.pdf:
	pdflatex $<
	pdflatex $<
	

all:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.log *.aux

.PHONY: all clean
