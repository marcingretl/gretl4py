gretl4py: Python Bindings for gretl
===================================

This package provides Python bindings to the ``gretl`` econometrics library via its official C API.

For full documentation, please visit the `gretl4py project page <https://gretl.sourceforge.net/gretl4py.html>`_.

.. note::

   The official PDF documentation is still under development and requires updates and enhancements.

   You can find useful example scripts in the ``demo/`` subdirectory (`view on SourceForge <https://sourceforge.net/p/gretl/gretl4py/ci/v0.2/tree/demo/>`_).

Package Contents
----------------

1. ``libgretl`` (including plugins) and its dependencies
2. ``gretl4py`` binary module bridging Python with ``libgretl`` via its official API
3. Python extensions supporting the binary module
4. Example Python scripts in the ``examples/`` directory
5. Sample datasets in the ``data/`` directory

Provided Functionality
----------------------

**Available Estimators:**

::

   ar, ar1, arima, biprobit, dpanel, duration, garch, heckit, hsk,
   intreg, lad, logit, logistic, midasreg, mpols, negbin, ols, panel,
   poisson, probit, quantreg, tobit, tsls, wls, var, vecm

**Available Tests:**

::

   add, adf, arch, autocorr, bds, bkw, breusch-pagan, chow, coeffsum, coint, comfac,
   cusum, difftest, johansen, kpss, leverage, levinlin, logs, meantest, normality,
   normtest, omit, panel, panspec, qlrtest, reset, restrict, runs, squares,
   white, white-nocross, vartest, vif, xdepend

.. note::

   Some tests are dataset-based, while others are model-based.

Usage
-----

1. Loading Datasets
~~~~~~~~~~~~~~~~~~~

Supported formats: ``.gdt``, ``.gdtb``, ``.csv``, ``.dta``, ``.wf1``, ``.xls``, ``.xlsx``, ``.ods``

Use ``get_data()`` to load a dataset:

.. code-block:: python

   import importlib.resources as resources
   import gretl

   data_dir = resources.files('gretl').joinpath('data')
   d1 = gretl.get_data(str(data_dir.joinpath('bjg.gdt')))

.. note::

   The first dataset loaded is automatically set as the default.

**Bundled Datasets:**

::

   abdata.gdt, bjg.gdt, data9-7.gdt, greene19_1.gdt, grunfeld.gdt, mroz87.gdt,
   rac3d.gdt, b-g.gdt, data4-10.gdt, denmark.gdt, greene22_2.gdt, kennan.gdt,
   ooballot.gdt, tobit.gdt, bjg.csv, data4-1.gdt, gdp_midas.gdt, greene25_1.gdt,
   longley.csv, penngrow.gdt, wtp.gdt

2. Estimating Models
~~~~~~~~~~~~~~~~~~~~

Basic usage pattern:

.. code-block:: python

   m = gretl.ESTIMATOR()
   m.fit()

To pass a dataset explicitly, use the ``data=...`` keyword argument.

**Example: OLS Regression**

.. code-block:: python

   m1 = gretl.ols(formula='g ~ const + lg').fit()
   print(m1)

Examples
--------

Example scripts are located in ``examples/estimators/`` and include:

::

   ar1.py, biprobit.py, heckit.py, logit.py, ols.py, probit.py, wls.py,
   arima.py, duration.py, lad.py, mpols.py, panel.py, tobit.py, ar.py,
   garch.py, logistic.py, negbin.py, poisson.py, quantreg.py, tsls.py

To view the source of ``ols.py``:

.. code-block:: python

   import inspect
   import gretl.examples.estimators.ols

   print(inspect.getsource(gretl.examples.estimators.ols.run_example))

To run the example:

.. code-block:: python

   import gretl.examples.estimators.ols

   gretl.examples.estimators.ols.run_example()

API Overview
------------

``class _gretl.Dataset``
~~~~~~~~~~~~~~~~~~~~~~~~

**Attributes:**

- ``is_default``
- ``source``

**Methods:**

::

   __copy__, __repr__, bwfilt, bkfilt, get_accessor, get_id, get_series,
   hpfilt, linked_models_list, new_list, new_series, print, sample,
   setobs, set_as_default, test, to_dict, to_file, varnames

``class _gretl.Model``
~~~~~~~~~~~~~~~~~~~~~~

**Methods:**

::

   fcast, fit, get_accessor, get_formula

``class _gretl.Model_NSE``
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Methods:**

::

   restrict, test

``class _gretl.Model_GretlModel_VAR``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Methods:**

::

   irf, test

``class _gretl.GretlModel_VAR_VECM``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Methods:**

::

   restrict
