

.SUFFIXES: .pdf .tex

TARGETS= ws-quick-guide.pdf

.tex.pdf:
	pdflatex $<
	pdflatex $<
	

all doc:	$(TARGETS)

clean:
	rm -f $(TARGETS) *.log *.aux

.PHONY: all doc clean
