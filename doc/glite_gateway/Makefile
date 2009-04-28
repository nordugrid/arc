

.SUFFIXES: .pdf .tex

TARGETS= glite_gateway.pdf

.tex.pdf:
	pdflatex $<
	pdflatex $<
	

all:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.log *.aux

.PHONY: all clean
