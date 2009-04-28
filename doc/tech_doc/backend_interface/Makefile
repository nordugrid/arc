
TARGET=Backends-arc1

all:	$(TARGET).pdf

$(TARGET).pdf: $(TARGET).tex
	pdflatex $(TARGET).tex
	pdflatex $(TARGET).tex

clean:
	rm -f $(TARGET).pdf *.log *.aux
