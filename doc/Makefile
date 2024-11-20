TEXDIR = $(topsrc)/doc

PKG = gretl4py
SRCS = $(PKG).tex $(PKG).bib

$(PKG).pdf: $(SRCS)
	@pdflatex $(PKG) && bibtex $(PKG) && makeindex $(PKG) && pdflatex $(PKG)
	@if grep 'Rerun to get' $(PKG).log ; then pdflatex $(PKG) ; fi

clean:
	@rm -f *.log *.aux *.bbl *.blg *.toc *.out $(PKG).pdf
