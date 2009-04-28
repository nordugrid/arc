

.SUFFIXES: .pdf .tex

TARGETS= arc-storage-documentation.pdf arc-storage-manual.pdf

.tex.pdf:
	pdflatex $<
	pdflatex $<
	

all doc:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.log *.aux

.PHONY: all clean doc
