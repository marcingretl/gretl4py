```text
                _   _   ___
               | | | | /   |
  __ _ _ __ ___| |_| |/ /| |_ __  _   _
 / _` | '__/ _ \ __| / /_| | '_ \| | | |
| (_| | | |  __/ |_| \___  | |_) | |_| |
 \__, |_|  \___|\__|_|   |_/ .__/ \__, |
  __/ |                    | |     __/ |
 |___/                     |_|    |___/
```

# gretl4py: Python Bindings for gretl

**Python bindings for the [gretl](https://gretl.sourceforge.net/) econometrics library**

**gretl4py** provides a Python interface to **libgretl**, the numerical and econometric engine underlying gretl. It brings gretl's econometric functionality, numerical routines, and data-handling capabilities into Python while providing Python-oriented interfaces for matrices, datasets, models, and other gretl objects.

gretl4py is **not a separate econometrics library**. It is a bridge between Python and `libgretl`: estimation and many numerical operations are performed by the mature gretl engine, while Python provides the programming environment and ecosystem around it.

> **Windows users:** gretl4py requires the **Microsoft Visual C++ 2022 Redistributable (x64)**.  
> Download it from: <https://aka.ms/vs/17/release/vc_redist.x64.exe>

---

## Highlights

gretl4py provides:

- **Econometric estimation**, including OLS, weighted and least-absolute-deviation regression, IV/2SLS, maximum likelihood, GMM, quantile regression, regularized regression, and various limited-dependent-variable models.
- **Time-series and multivariate methods**, including AR/ARIMA, GARCH-type models, VAR and VECM models, structural VAR functionality, and related tests.
- **Panel-data methods**, including dynamic-panel and instrumental-variable estimators.
- **Mixed-frequency modelling**, including MIDAS regression.
- **Econometric tests**, covering unit roots, cointegration, heteroskedasticity, autocorrelation, specification, parameter restrictions, structural stability, and other commonly used procedures.
- **Native gretl matrices and datasets**, exposed as Python objects with Python-friendly indexing and conversion facilities.
- **Bundles and other gretl objects**, allowing Python code to interact directly with objects used by `libgretl`.
- **NumPy and pandas interoperability**, including conversion to NumPy arrays and pandas DataFrames and, where appropriate, zero-copy views of matrix data.
- **gretl's hansl language**, allowing Python programs to execute hansl code and exchange user-defined variables with `libgretl`.
- **gretl function packages**, making it possible to use functionality implemented in hansl packages from the Python environment.
- **LIBSVM-based machine-learning functionality** available through gretl.

The package is designed to allow users to combine the econometric functionality of gretl with the broader Python ecosystem, including NumPy, pandas, matplotlib, and other scientific and statistical libraries.

---

## Installation

The recommended way to install gretl4py is with `pip`:

```bash
pip install gretl4py
```

Pre-built packages are available for supported Python versions and platforms.

For platform-specific requirements and available distributions, see the [gretl4py project page](https://gretl.sourceforge.net/gretl4py.html).

---

## A quick example

A typical gretl4py workflow consists of loading a dataset, estimating a model, and working with the resulting Python object:

```python
import gretl

data = gretl.get_data("bjg.gdt")

model = gretl.ols(
    [1, 0, 2],
    data=data
).fit()

print(model)
```

The exact estimator interface depends on the model being estimated. The `gretl` module exposes both high-level estimator functions and corresponding gretl model objects.

---

## Working with datasets

gretl datasets are represented by the `gretl.Dataset` class. Datasets can be loaded from a number of formats supported by gretl, including:

- `.gdt`
- `.gdtb`
- `.csv`
- `.dta`
- `.wf1`
- `.xls`
- `.xlsx`
- `.ods`

For example:

```python
import importlib.resources as resources
import gretl

data_dir = resources.files("gretl").joinpath("data")
data = gretl.get_data(str(data_dir.joinpath("bjg.gdt")))

print(data)
```

Datasets can be manipulated directly from Python, including adding and transforming series, working with lists, and passing datasets to estimators.

gretl4py also provides conversion to pandas:

```python
df = data.to_pandas()
```

---

## Matrices

The `gretl.Matrix` class provides a Python interface to gretl's native matrix objects.

Matrices support Python-style indexing and slicing, matrix operations, row and column names, and conversion to NumPy.

For example:

```python
import gretl

m = gretl.Matrix.ones(3, 3)
m[0, 0] = 10

print(m)
```

NumPy interoperability is available both through copying and through zero-copy views where appropriate:

```python
a = m.to_numpy()       # independent NumPy array
v = m.numpy_view       # view of the underlying gretl matrix
```

A pandas representation is also available:

```python
df = m.to_pandas()
```

The distinction between copies and views is important when modifying data; see the documentation for details.

---

## Estimation

gretl4py exposes a range of gretl estimators through Python. Depending on the model, these include:

- OLS and weighted least squares
- AR and ARIMA
- GARCH-type models
- IV / 2SLS
- LAD and quantile regression
- logit, probit, tobit and related limited-dependent-variable models
- count and duration models
- sample-selection models
- panel-data estimators
- dynamic-panel estimators
- MIDAS regression
- VAR and VECM
- structural VAR models
- regularized regression, including LASSO, Ridge and Elastic Net
- maximum-likelihood and GMM-based estimation

The resulting objects are instances of gretl4py model classes and provide access to estimation results, coefficients, covariance matrices, residuals, fitted values, forecasts, tests, and other model-specific information.

For example:

```python
import gretl

data = gretl.get_data("bjg.gdt")

model = gretl.ols([1, 0, 2], data=data).fit()

print(model.coeff)
print(model.vcv)
```

The available methods depend on the particular model class. For example, VAR and VECM models provide functionality specific to multivariate time-series analysis, including impulse-response analysis.

---

## Econometric tests

Many of gretl's statistical and econometric tests are available directly from Python. These include tests for, among other things:

- unit roots and stationarity,
- cointegration,
- autocorrelation,
- heteroskedasticity,
- normality,
- structural stability,
- parameter restrictions,
- specification,
- omitted variables,
- functional form,
- multicollinearity,
- panel-data dependence and specification.

Tests may operate either on a dataset or on a fitted model, depending on the particular procedure.

---

## NumPy and pandas interoperability

gretl4py is intended to work naturally with the Python scientific-computing ecosystem.

For matrices, the following interfaces are provided:

```python
numpy_array = matrix.to_numpy()
numpy_view = matrix.numpy_view

dataframe = matrix.to_pandas()
```

`to_numpy()` and `to_pandas()` create independent objects. In contrast, `numpy_view` exposes the underlying matrix storage directly and therefore avoids a data copy.

Datasets can be converted to pandas DataFrames with:

```python
dataframe = dataset.to_dataframe()
```

This makes it possible to use gretl for estimation while using NumPy, pandas, matplotlib, or other Python packages for subsequent processing and visualization.

---

## Interoperability with hansl

gretl4py also provides an additional interoperability layer with **hansl**, gretl's scripting language.

Python code can execute hansl code directly:

```python
import gretl

gretl.run_hansl("""
function void foo (void)
    print "Hello from hansl"
end function

foo()
""")
```

An existing hansl script can be executed with:

```python
gretl.run_script("my_script.inp")
```

This functionality is particularly useful for testing existing hansl code, using functionality implemented in gretl function packages, and accessing parts of gretl that are naturally expressed in hansl.

### User-defined variables

gretl4py can also exchange **user-defined variables** with `libgretl`.

For example, a matrix created in Python can be placed in the gretl namespace:

```python
import gretl

m = gretl.Matrix.ones(2, 2)

gretl.genr(name="mat", value=m)
gretl.run_hansl("mat = mat .* 2")

m = gretl.get_uservar("mat")
```

These variables live in the `libgretl` namespace rather than in Python's namespace. Consequently, they can be accessed by subsequently executed hansl code and can also be retrieved from Python.

> **Important:** hansl execution and user-defined-variable interoperability are supplementary features of gretl4py. They are provided primarily to facilitate interoperability with existing gretl/hansl code and should not be regarded as the primary programming interface of the package. For new Python applications, the native gretl4py API is generally preferable.

---

## Examples and demonstrations

The source distribution contains a growing collection of examples and demonstrations.

### Estimator examples

Estimator examples are located in:

```text
gretl/examples/estimators/
```

They provide small, self-contained examples of individual estimators. For example:

```python
import gretl.examples.estimators.ols

gretl.examples.estimators.ols.run_example()
```

The source of an example can also be inspected directly from Python:

```python
import inspect
import gretl.examples.estimators.ols

print(inspect.getsource(
    gretl.examples.estimators.ols.run_example
))
```

### Feature demonstrations

More extensive demonstrations are located in:

```text
demo/
```

These cover not only estimation, but also data handling, matrices, bundles, user-defined variables, hansl interoperability, forecasting, VAR/VECM functionality, packages, filters, nonlinear models, and other gretl4py features.

The examples and demonstrations are often the best starting point for seeing how a particular feature is intended to be used.

---

## Package contents

A gretl4py installation contains:

1. **`libgretl` and its plugins** together with the required runtime dependencies;
2. the compiled **`_gretl` Python extension module** providing the binding to `libgretl`;
3. Python-level helper modules and utilities;
4. example modules and scripts;
5. bundled datasets and other supporting resources.

The Python extension is implemented using **C++ and pybind11**, while the underlying econometric functionality is provided by `libgretl`.

---

## Documentation

The main documentation is available at:

<https://gretl.sourceforge.net/gretl4py.html>

The current PDF documentation is:

<https://sourceforge.net/projects/gretl/files/gretl4py/gretl4py.pdf/download>

The documentation is actively evolving alongside the Python API.

The source code, examples, demonstrations, and development history are available in the gretl4py repository:

<https://sourceforge.net/p/gretl/gretl4py/ci/master/tree/>

---

## License

gretl4py is distributed under the **GNU General Public License, version 3 or later (GPL-3.0-or-later)**.

gretl and its components are distributed under their respective free-software licenses. See the accompanying license files and the gretl documentation for details.

---

## Development status

gretl4py is under active development. The Python interface continues to evolve as more of the functionality provided by `libgretl` is exposed through a native Python API.

The project aims to keep the Python interface consistent and Pythonic while retaining close correspondence with gretl's established econometric functionality.

For current functionality, examples, and API details, consult the documentation and source tree rather than relying solely on this README.
