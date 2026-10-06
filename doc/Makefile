PKG = gretl4py

LATEX     = pdflatex
BIBTEX    = bibtex
MAKEINDEX = makeindex

CHAPTERS = \
	Introduction.tex \
	Installation.tex \
	Implementation.tex \
	Usage_of_the_package.tex \
	Examples.tex

SOURCES = \
	class_py_bundle.tex \
	class_py_dataset.tex \
	class_py_matrix.tex \
	class_py_model.tex \
	class_py_package.tex \
	data_management.tex \
	estimators.tex \
	examples_bayesian.tex \
	examples_nontrivial_econometrics.tex \
	examples_poe4th.tex \
	examples_statsmodels.tex \
	gretldataset_pydict.tex \
	gretlmodel_hierarchy.tex \
	implementation_python_side.tex \
	module_related_tools_usage.tex \
	objects_life_cycle.tex \
	running_hansl.tex \
	sourcetree.tex \
	usage.tex \
	usage_contributed_packages.tex \
	usage_forecasting.tex \
	usage_native_estimators.tex

SUPPORT = \
	gretl4pydoc.cls \
	gretlwp.sty \
	$(PKG).bib

SRCS = $(PKG).tex $(CHAPTERS) $(SOURCES) $(SUPPORT)

PDF = $(PKG).pdf

.PHONY: all rebuild clean distclean

all: $(PDF)

$(PDF): $(SRCS)
	$(LATEX) $(PKG)
	$(BIBTEX) $(PKG)
	$(MAKEINDEX) $(PKG)
	$(LATEX) $(PKG)
	$(LATEX) $(PKG)

rebuild:
	$(LATEX) $(PKG)
	$(BIBTEX) $(PKG)
	$(MAKEINDEX) $(PKG)
	$(LATEX) $(PKG)
	$(LATEX) $(PKG)

clean:
	rm -f *.bbl *.blg *.ilg *.ind *.idx *.lof *.log *.out \
	      *.toc *.synctex.gz

distclean: clean
	rm -f *.aux $(PDF)
