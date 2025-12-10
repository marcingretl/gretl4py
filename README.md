# gretl4py: Python Bindings for gretl
                _   _   ___
               | | | | /   |
  __ _ _ __ ___| |_| |/ /| |_ __  _   _
 / _` | '__/ _ \ __| / /_| | '_ \| | | |
| (_| | | |  __/ |_| \___  | |_) | |_| |
 \__, |_|  \___|\__|_|   |_/ .__/ \__, |
  __/ |                    | |     __/ |
 |___/                     |_|    |___/


**gretl4py** provides Python bindings to **gretl**, a free, open-source, cross-platform software package for econometric analysis.
Gretl is known for its numerical accuracy and computational efficiency — capabilities that gretl4py inherits directly through the underlying `libgretl` engine.

gretl4py is not a standalone econometrics library. Instead, it serves as a **bridge** between Python and `libgretl`, exposing a high-performance backend that supports:

1. **Econometric estimation:** least squares, maximum likelihood, GMM; single-equation and system estimators; regularized regression (LASSO, Ridge, Elastic Net)
2. **Time-series methods:** ARIMA, numerous univariate GARCH-type models, VARs and VECMs (including structural VARs), unit-root/cointegration tests, Kalman filter, etc.
3. **Limited dependent variables:** logit, probit, tobit, sample selection, interval regression, count and duration models, etc.
4. **Panel-data estimators:** including IV, probit, and GMM-based dynamic panel methods
5. **Mixed-frequency (MIDAS) models**
6. **Machine learning support via LIBSVM**
7. **The hansl scripting language**, enabling users to write gretl function packages — collections of hansl-written functions for advanced econometric analysis
   <https://gretl.sourceforge.net/function_packages.html>

Please note that some features available in gretl (e.g., Kalman filtering) are **not yet implemented** in gretl4py, but are planned.
For full documentation, visit the gretl4py project page:
<https://gretl.sourceforge.net/gretl4py.html>

> **Note**
> The official PDF documentation is under development and still requires updates.
> Example scripts are available in the `demo/` directory:
> <https://sourceforge.net/p/gretl/gretl4py/ci/v0.2/tree/demo/>

---

## Package Contents

1. `libgretl` (including plugins) and its dependencies
2. The `gretl4py` binary module providing the Python–C interface
3. Additional Python helper modules
4. Example scripts in `examples/`
5. Sample datasets in `data/`

---

## Provided Functionality

### **Available Estimators**

:: ar, ar1, arima, biprobit, dpanel, duration, garch, heckit, hsk,
   intreg, lad, logit, logistic, midasreg, mpols, negbin, ols, panel,
   poisson, probit, quantreg, tobit, tsls, wls, var, vecm


### **Available Tests**

:: add, adf, arch, autocorr, bds, bkw, breusch-pagan, chow, coeffsum, coint, comfac,
   cusum, difftest, johansen, kpss, leverage, levinlin, logs, meantest, normality,
   normtest, omit, panel, panspec, qlrtest, reset, restrict, runs, squares,
   white, white-nocross, vartest, vif, xdepend

> **Note**
> Some tests operate on datasets, while others are model-based.

---

## Usage

### 1. Loading Datasets

Supported formats:
`.gdt`, `.gdtb`, `.csv`, `.dta`, `.wf1`, `.xls`, `.xlsx`, `.ods`

Use `get_data()` to load a dataset:

```python
import importlib.resources as resources
import gretl

data_dir = resources.files('gretl').joinpath('data')
d1 = gretl.get_data(str(data_dir.joinpath('bjg.gdt')))
```

**Bundled Datasets:**

:: abdata.gdt, bjg.gdt, data9-7.gdt, greene19_1.gdt, grunfeld.gdt, mroz87.gdt,
   rac3d.gdt, b-g.gdt, data4-10.gdt, denmark.gdt, greene22_2.gdt, kennan.gdt,
   ooballot.gdt, tobit.gdt, bjg.csv, data4-1.gdt, gdp_midas.gdt, greene25_1.gdt,
   longley.csv, penngrow.gdt, wtp.gdt

2. Estimating Models

Basic usage pattern:

```python
m = gretl.ESTIMATOR()
m.fit()
```
where ``ESTIMATOR()`` is any supported estimator.
To use a specific dataset, supply it via ``data=...`` keyword argument.

## **Examples**

Example scripts are located in ``examples/estimators/`` and include:

:: ar1.py, biprobit.py, heckit.py, logit.py, ols.py, probit.py, wls.py,
   arima.py, duration.py, lad.py, mpols.py, panel.py, tobit.py, ar.py,
   garch.py, logistic.py, negbin.py, poisson.py, quantreg.py, tsls.py

**Example: OLS Regression**

To view the source of ols.py

```python
import inspect
import gretl.examples.estimators.ols

print(inspect.getsource(gretl.examples.estimators.ols.run_example))
```

To run the example:
```python
import gretl.examples.estimators.ols
gretl.examples.estimators.ols.run_example()
```
