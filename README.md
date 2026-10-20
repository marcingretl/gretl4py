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

Python bindings for the [gretl](https://gretl.sourceforge.net/) econometrics library.

gretl4py provides a Python interface to **libgretl**, the numerical and econometric engine underlying gretl. It brings gretl's econometric functionality, numerical routines, and data-handling capabilities into Python while providing Python-oriented interfaces for matrices, datasets, models, bundles, and other gretl objects.

gretl4py is not a separate econometrics library. It is a bridge between Python and `libgretl`: estimation and many numerical operations are performed by the mature gretl engine, while Python provides the programming environment and ecosystem around it.

The overall architecture is:

```text
+------------------+      +----------------------+      +------------------+
|      Python      | <--> |       gretl4py       | <--> |     libgretl     |
+------------------+      +----------------------+      +------------------+
|                         |                             |
| User writes             | C++ bindings via            | C API for data,
| Python code             | pybind11                    | econometrics,
|                         |                             | models, matrices,
| Python objects          | Conversion between          | bundles, function
| (NumPy, pandas,         | Python and libgretl         | packages, etc.
| lists, dicts, etc.)     | types and objects           |
|                         |                             |
```

> **Supported Python versions:** 3.11, 3.12, 3.13, and 3.14.
>
> **Windows users:** gretl4py requires the appropriate Microsoft Visual C++ 2022
> Redistributable (v14.x). See Microsoft's
> [latest supported Visual C++ Redistributable downloads](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).
> On Windows x86-64 (AMD64), MPI functionality additionally requires
> [Microsoft MPI (MS-MPI)](https://learn.microsoft.com/en-us/message-passing-interface/microsoft-mpi),
> installed separately. MS-MPI is not required for ordinary, non-MPI use of gretl4py.

---

## Highlights

gretl4py provides:

- Econometric estimation, including OLS, weighted and least-absolute-deviation regression, IV/2SLS, maximum likelihood, GMM, quantile regression, regularized regression, and various limited-dependent-variable models.
- Time-series and multivariate methods, including AR/ARIMA, GARCH-type models, VAR and VECM models, structural VAR functionality, and related tests.
- Panel-data methods, including dynamic-panel and instrumental-variable estimators.
- Mixed-frequency modelling, including MIDAS regression.
- Econometric tests, covering unit roots, cointegration, heteroskedasticity, autocorrelation, specification, parameter restrictions, structural stability, and other commonly used procedures.
- Native gretl matrices and datasets, exposed as Python objects with Python-friendly indexing and conversion facilities.
- Bundles and other gretl objects, allowing Python code to interact directly with objects used by `libgretl`.
- NumPy and pandas interoperability, including conversion to NumPy arrays and pandas DataFrames and, where appropriate, zero-copy views of matrix data.
- gretl's hansl language, allowing Python programs to execute hansl code and exchange user-defined variables with `libgretl`.
- **gretl function packages**, making functionality implemented in hansl packages directly usable from Python.
- LIBSVM-based machine-learning functionality available through gretl.
- Plotting support integrated with matplotlib.

The package is designed to allow users to combine the econometric functionality of gretl with the broader Python ecosystem, including NumPy, pandas, matplotlib, and other scientific and statistical libraries.

---

## Installation

For normal use, installing a pre-built gretl4py package is recommended.

### PyPI

On GNU/Linux:

```bash
python3 -m pip install gretl4py
```

On macOS, use a user installation:

```bash
python3 -m pip install --user gretl4py
```

On MS Windows:

```bash
python.exe -m pip install gretl4py
```

### Conda

Pre-built gretl4py packages are also available through Conda:

```bash
conda install -c marcin_gretlconference.org gretl4py
```

### Development snapshots

Pre-built development snapshots are available from the
[gretl4py snapshots area](https://sourceforge.net/projects/gretl/files/gretl4py/snapshots/).

Download the wheel matching the Python version, operating system, and architecture,
then install it with `pip`. On macOS, add the `--user` option.

### Verifying the installation

On GNU/Linux or macOS:

```bash
python3 -c "import gretl; gretl.about(True)"
```

On MS Windows:

```bash
python.exe -c "import gretl; gretl.about(True)"
```

For platform-specific requirements and distributions, see the
[gretl4py project page](https://gretl.sourceforge.net/gretl4py.html).

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

The exact estimator interface depends on the model being estimated. The `gretl`
module exposes both high-level estimator functions and corresponding gretl model
objects.

---

## Working with datasets

gretl datasets are represented by the `gretl.Dataset` class. Datasets can be loaded
from a number of formats supported by gretl, including:

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

Datasets can be manipulated directly from Python, including adding and transforming
series, working with lists, and passing datasets to estimators.

gretl4py also provides conversion to pandas:

```python
df = data.to_pandas()
```

---

## Matrices

The `gretl.Matrix` class provides a Python interface to gretl's native matrix objects.

Matrices support:

- Python-style indexing and slicing;
- matrix operations;
- row and column names;
- conversion to NumPy;
- conversion to pandas;
- zero-copy NumPy views where appropriate.

For example:

```python
import gretl

m = gretl.Matrix.ones(3, 3)
m[0, 0] = 10

print(m)
```

NumPy interoperability is available both through copying and through zero-copy views:

```python
a = m.to_numpy()       # independent NumPy array
v = m.numpy_view       # view of the underlying gretl matrix
```

A pandas representation is also available:

```python
df = m.to_pandas()
```

The distinction between copies and views is important when modifying data; see the
documentation for details.

---

## Estimation

gretl4py exposes a range of gretl estimators through Python. Estimator functions return
model objects; estimation is performed by calling `.fit()`. Depending on the model,
the available estimators include:

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

The resulting objects are instances of gretl4py model classes and provide access to
estimation results, coefficients, covariance matrices, residuals, fitted values,
forecasts, tests, and other model-specific information.

For example:

```python
import gretl

data = gretl.get_data("bjg.gdt")

model = gretl.ols([1, 0, 2], data=data).fit()

print(model.coeff)
print(model.vcv)
```

The available methods depend on the particular model class. For example, VAR and VECM
models provide functionality specific to multivariate time-series analysis,
including impulse-response analysis.

---

## Econometric tests

Many of gretl's statistical and econometric tests are available directly from Python.
These include tests for, among other things:

- unit roots and stationarity;
- cointegration;
- autocorrelation;
- heteroskedasticity;
- normality;
- structural stability;
- parameter restrictions;
- specification;
- omitted variables;
- functional form;
- multicollinearity;
- panel-data dependence and specification.

Tests may operate either on a dataset or on a fitted model, depending on the
particular procedure.

---

## Function packages

One of gretl's distinctive features is its **function-package system**. Function
packages extend gretl by providing additional functionality implemented in hansl,
gretl's scripting language.

gretl4py provides a bridge to this ecosystem: **functions defined in gretl function
packages can be loaded and used as Python functions**. This means that the Python
interface is not limited to functionality implemented directly in the gretl4py
bindings. A gretl function package can effectively extend the Python API.

### Loading a package

A package is loaded with `gretl.include()`:

```python
import gretl

gretl.pkg("install", "BMA")
BMA = gretl.include("BMA")
```

The returned package object can be inspected, for example:

```python
BMA.show_functions()
```

After a package has been included, its functions are available through the `gretl`
module. For example:

```python
import gretl

BMA = gretl.include("BMA")

data = gretl.get_data("FLS_41t72.gdt", frompkg="BMA")

opt = gretl.Bundle()
opt["Nrep"] = 10**5
opt["model_prior"] = "binomial"
opt["Nrank"] = 5

result = gretl.BMA(
    Y="GDP_growth",
    X_list=list(range(2, 14)),
    quiet=True,
    Options=opt
)

gretl.BMA_Print(result)
```

The important point is that `BMA()` and `BMA_Print()` are functions supplied by the
gretl function package rather than native gretl4py bindings.

### Package functions and gretl objects

Package functions can exchange gretl4py objects with the underlying hansl code. For
example, a package function can receive a `gretl.Matrix`, a NumPy view, a `gretl.Bundle`,
a dataset, or other supported gretl objects.

For example, the `extra` package can be used with both a gretl matrix and its NumPy
view:

```python
import gretl

extra = gretl.include("extra")

m = gretl.Matrix(4, 1, "normal")

print(gretl.combinations(m, 1))
print(gretl.combinations(m.numpy_view, 1))
```

This allows package functions to participate naturally in Python workflows while
retaining the functionality implemented in the original hansl package.

### Packages demonstrated by gretl4py

The repository currently contains package examples for:

| Package | Main functionality |
|---|---|
| `BACE` | Bayesian Averaging of Classical Estimates |
| `BayTool` | Bayesian regression methods |
| `BMA` | Bayesian Model Averaging |
| `BVAR` | Bayesian VAR models |
| `criteria` | Model-selection criteria |
| `gig` | GARCH models |
| `ParMA` | Parallel Bayesian Model Averaging |
| `SVAR` | Structural VAR models |
| `TVC` | Time-varying coefficient models |

The corresponding examples are located in:

```text
gretl/examples/packages/
```

The repository also contains a combined package demonstration in:

```text
demo/packages.py
```

These examples are intended to show not only how a package is loaded, but also how
package functions interact with native gretl4py objects.

### Package ecosystem

The gretl function-package ecosystem is considerably larger than the examples
included with gretl4py. It contains official gretl add-ons as well as a large
collection of contributed packages covering, among other areas:

- Bayesian methods and model averaging;
- unit-root, stationarity and structural-break tests;
- cointegration and long-run analysis;
- univariate time-series models;
- VAR, SVAR and factor models;
- volatility and financial econometrics;
- panel-data models;
- discrete and limited-dependent-variable models;
- hypothesis testing and diagnostics;
- estimation and regression tools;
- forecasting and prediction;
- machine learning and nonparametric methods;
- spatial econometrics;
- graphics and visualization;
- data access and management;
- programming utilities and GUI tools.

The package mechanism is therefore an important part of the overall gretl4py
architecture: it allows the Python interface to benefit from the existing and
continuously growing gretl function-package ecosystem without requiring every
package to be reimplemented as a separate Python extension.

---

## NumPy and pandas interoperability

gretl4py is intended to work naturally with the Python scientific-computing ecosystem.

For matrices, the following interfaces are provided:

```python
numpy_array = matrix.to_numpy()
numpy_view = matrix.numpy_view

dataframe = matrix.to_pandas()
```

`to_numpy()` and `to_pandas()` create independent objects. In contrast, `numpy_view`
exposes the underlying matrix storage directly and therefore avoids a data copy.

Datasets can be converted to pandas DataFrames with:

```python
dataframe = dataset.to_pandas()
```

This makes it possible to use gretl for estimation while using NumPy, pandas,
matplotlib, or other Python packages for subsequent processing and visualization.

---

## Interoperability with hansl

gretl4py also provides an additional interoperability layer with hansl, gretl's
scripting language.

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

This functionality is particularly useful for testing existing hansl code, accessing
functionality implemented in gretl function packages, and using parts of gretl that
are naturally expressed in hansl.

### User-defined variables

gretl4py can also exchange user-defined variables with `libgretl`.

For example, a matrix created in Python can be placed in the gretl namespace:

```python
import gretl

m = gretl.Matrix.ones(2, 2)

gretl.genr(name="mat", value=m)
gretl.run_hansl("mat = mat .* 2")

m = gretl.get_uservar("mat")
```

These variables live in the `libgretl` namespace rather than in Python's namespace.
Consequently, they can be accessed by subsequently executed hansl code and can also
be retrieved from Python.

> **Important:** hansl execution and user-defined-variable interoperability are
> supplementary features of gretl4py. They are provided primarily to facilitate
> interoperability with existing gretl/hansl code and should not be regarded as the
> primary programming interface of the package. For new Python applications, the
> native gretl4py API is generally preferable.

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

### Package examples

Examples demonstrating gretl function packages are located in:

```text
gretl/examples/packages/
```

They provide focused examples of loading and using individual packages.

### Feature demonstrations

More extensive demonstrations are located in:

```text
demo/
```

These cover not only estimation, but also data handling, matrices, bundles,
user-defined variables, hansl interoperability, forecasting, VAR/VECM functionality,
function packages, filters, nonlinear models, and other gretl4py features.

The examples and demonstrations are often the best starting point for seeing how a
particular feature is intended to be used.

---

## Package contents

A gretl4py installation contains:

1. the compiled `_gretl` Python extension module providing the binding to `libgretl`;
2. `libgretl` and the gretl plugins bundled for use by gretl4py;
3. Python-level helper modules and utilities;
4. example modules and scripts;
5. bundled datasets and other supporting resources;
6. the `COPYING` file containing the GNU GPL v3 license text;
7. the `gretlmpi` executable (`gretlmpi.exe` on Windows), unless MPI staging is disabled.

The Python extension is implemented using C++ and pybind11, while the underlying
econometric functionality is provided by `libgretl`. Some functionality provided by
individual gretl plugins may require additional runtime libraries; the exact requirements
depend on the platform and the functionality being used.

### MPI support

The `gretlmpi` executable is included when MPI support files are enabled.
The MPI runtime itself is platform-dependent:

- **GNU/Linux:** `gretlmpi` uses the system MPI installation; a bundled
  `gretl/mpi/` directory is not required.
- **macOS:** the package can include a bundled Open MPI runtime under `gretl/mpi/`.
- **Windows x86-64:** `gretlmpi.exe` requires an external Microsoft MPI (MS-MPI)
  installation.

When building from source, `meson install -C build` installs the prepared
`gretlmpi` executable and any staged MPI runtime files. Set `-DNOMPI=true`
to omit these MPI support files.

---

## Building from source

For normal use, pre-built packages are recommended. Source builds use Meson and Ninja
and require a suitable `libgretl` development environment.

On MS Windows (x86-64 and ARM64), native builds use the Microsoft Windows SDK,
LLVM/Clang, and the
[libgretl for MS SDK](https://sourceforge.net/projects/libgretl-for-ms-sdk/) stack.
MSVC cannot currently be used to build gretl4py because of an ABI incompatibility
arising from the representation of complex numbers. Additional libraries required by
individual gretl plugins must be supplied separately where needed.

The PDF manual contains the complete and current build instructions for GNU/Linux,
macOS, and MS Windows.

---

## Documentation

The main documentation is available at:

<https://gretl.sourceforge.net/gretl4py.html>

The current PDF documentation is available from the
[SourceForge files area](https://sourceforge.net/projects/gretl/files/gretl4py/gretl4py.pdf/download).

The documentation is actively evolving alongside the Python API.

The source code, examples, demonstrations, and development history are available in
the [gretl4py repository](https://sourceforge.net/p/gretl/gretl4py/).

---

## License

gretl4py is distributed under the GNU General Public License, version 3 or later
(GPL-3.0-or-later).

gretl and its components are distributed under their respective free-software
licenses. See the accompanying license files and the gretl documentation for details.

---

## Development status

gretl4py is under active development. The Python interface continues to evolve as
more of the functionality provided by `libgretl` is exposed through a native Python
API.

The project aims to keep the Python interface consistent and Pythonic while retaining
close correspondence with gretl's established econometric functionality.

For current functionality, examples, and API details, consult the documentation and
source tree rather than relying solely on this README.
