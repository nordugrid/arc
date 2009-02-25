
NAME=Guide

$(NAME).pdf: $(NAME).tex $(wildcard */*.tex)
	pdflatex $(NAME).tex
	$(MAKE) $(NAME).bbl
	pdflatex $(NAME).tex

$(NAME).bbl: literature.bib
	bibtex $(NAME)

clean:
	find . -name '*.aux' -o -name '*.bbl' -o -name '*.log' -o -name '*.blg' -o -name '*.toc' | xargs -r rm 

look:	$(NAME).pdf
	[ -x /usr/bin/kpdf ] && kpdf $< || [ -x /usr/bin/xpdf ] && xpdf $< || echo "Don't know how to open PDFs. Please edit this Makefile."


.PHONY: clean look
