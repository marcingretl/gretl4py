"""Regression tests for the ownership of series saved from fitted models.

Adapted from memory_leaks.py. These tests exercise the installed gretl4py
package without manually changing pybind11 reference counts.
"""


def test_save_yhat_replaces_named_series_without_growing_dataset():
    import gretl

    # Create a model using the public estimator factory function.
    dataset = gretl.get_data("bjg", quiet=True)
    model = gretl.ols(formula="lg const g", data=dataset).fit()
    original_k = dataset.k

    # save_yhat() returns a series name, not the fitted-value vector.
    name = "pytest_yhat"
    assert model.save_yhat(name) == name
    assert dataset.k == original_k + 1

    # Repeated writes must replace the named series, not append new series.
    for _ in range(100):
        assert model.save_yhat(name) == name
        assert dataset.k == original_k + 1

    # The saved values remain accessible through the public Dataset API.
    fitted = dataset.get_series(name)
    assert (fitted.rows, fitted.cols) == (dataset.n, 1)
