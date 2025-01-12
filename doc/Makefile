TEXDIR = $(topsrc)/doc

PKG = gretl4py
SRCS = class_py_model.tex \
	gretl4py.tex \
	installation.tex \
	sourcetree.tex \
	examples.tex \
	implementation_python_side.tex \
	introduction.tex \
	usage.tex \
	$(PKG).bib \
	gretl4pydoc.cls

$(PKG).pdf: $(SRCS)
	@pdflatex $(PKG) && bibtex $(PKG) && makeindex $(PKG) && pdflatex $(PKG)
	@if grep 'Rerun to get' $(PKG).log ; then pdflatex $(PKG) ; fi

clean:
	@rm -f *.log *.aux *.bbl *.blg *.toc *.out $(PKG).pdf
