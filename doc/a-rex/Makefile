
TARGET=arex_tech_doc

all:	$(TARGET).pdf

$(TARGET).pdf: $(TARGET).tex
	pdflatex $(TARGET).tex
	pdflatex $(TARGET).tex

clean:
	rm -f $(TARGET).pdf *.log *.aux
