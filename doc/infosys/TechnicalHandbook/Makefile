

.SUFFIXES: .pdf .tex


TARGETS=$(patsubst %.pdf,%.tex,$(wildcard ISIS*Handbook.tex))
all:	$(TARGETS)

$(TARGETS):	$(wildcard *.tex)
	pdflatex ISIS*.tex
	pdflatex ISIS*.tex
	
clean:
	rm -f ISIS*pdf *.log *.aux

.PHONY: all clean
