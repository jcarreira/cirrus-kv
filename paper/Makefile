.PHONY: clean clean-latex open spell

PROJ=main
TEXFILES=$(wildcard *.tex)
BIBFILE=bib/references.bib

LATEX=pdflatex
BIBTEX=bibtex

main.pdf: $(TEXFILES) $(BIBFILE)
	$(LATEX) main.tex
	$(BIBTEX) main
	$(LATEX) main.tex
	$(LATEX) main.tex

open: $(PROJ).pdf
	kpdf $^

clean-latex:
	rm -f $(PROJ).pdf *aux *.toc *.bbl *.blg *.log *.out *.bak *.cb *.cb2

spell:
	for i in $(TEXFILES) ; do \
	aspell -t -c -p ../dict.txt $$i ; \
	done;
