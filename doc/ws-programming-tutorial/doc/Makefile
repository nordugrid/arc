
NAME=Guide

$(NAME).pdf: $(NAME).tex $(wildcard */*.tex)
	pdflatex $(NAME).tex
	$(MAKE) $(NAME).bbl
	pdflatex $(NAME).tex

$(NAME).bbl: literature.bib
	bibtex $(NAME)

clean:
	find . -name '*.aux' -o -name '*.bbl' -o -name '*.log' -o -name '*.blg' -o -name '*.toc' | xargs -r rm 
