PKG = gretl4py

SRCS = \
	gretl4py.tex \
	gretl4pydoc.cls \
	class_py_bundle.tex \
	class_py_dataset.tex \
	class_py_matrix.tex \
	class_py_model.tex \
	class_py_package.tex \
	gretldataset_pydict.tex \
	gretlmodel_hierarchy.tex \
	installation.tex \
	introduction.tex \
	usage.tex \
	data_management.tex \
	estimators.tex \
	implementation_python_side.tex \
	objects_life_cycle.tex \
	module_related_tools_usage.tex \
	usage_contributed_packages.tex \
	usage_forecasting.tex \
	usage_native_estimators.tex \
	examples_bayesian.tex \
	examples_nontrivial_econometrics.tex \
	examples_poe4th.tex \
	examples_statsmodels.tex \
	$(PKG).bib \
	gretlwp.sty

PDF = $(PKG).pdf

all: $(PDF)

$(PDF): $(SRCS)
	pdflatex $(PKG)
	bibtex $(PKG)
	makeindex $(PKG)
	pdflatex $(PKG)
	pdflatex $(PKG)

clean:
	rm -f *.aux *.bbl *.blg *.ilg *.ind *.lof *.log *.out *.synctex.gz *.toc *.pdf

.PHONY: all clean
