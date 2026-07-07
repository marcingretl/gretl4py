import sys
import time

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import statsmodels.api as sm
import gretl

sys.path.append("../")
import helper_func as hf

np.random.seed(9876789)

"""
Selected OLS examples from the statsmodels documentation,
rewritten using the gretl4py API.

Original:
https://www.statsmodels.org/stable/examples/notebooks/generated/ols.html
"""

SHOW_STATSMODELS = True


# ============================================================
# 1. OLS estimation
# ============================================================

def example_ols():
    hf.printhead("OLS estimation")

    nsample = 100
    x = np.linspace(0, 10, nsample)
    X = np.column_stack((x, x**2))

    beta = np.array([1.0, 0.1, 10.0])
    e = np.random.normal(size=nsample)

    X_sm = sm.add_constant(X)
    y = X_sm @ beta + e

    # ---------------- statsmodels ----------------
    if SHOW_STATSMODELS:
        t1 = hf.benchmark_fit(lambda: sm.OLS(y, X).fit())

        sm_results = sm.OLS(y, X_sm).fit()
        print(sm_results.summary())

        hf.runtime(t1)

    # ---------------- gretl ----------------
    hf.linenr(5)

    d2 = gretl.get_data(gretl.nparray2dict(X, Y=y, with_const=False))
    t2 = hf.benchmark_fit(lambda: gretl.ols("y ~ const + x1 + x2", data=d2).fit())

    g_results = gretl.ols("y ~ const + x1 + x2", data=d2).fit()
    print(g_results)

    hf.runtime(t2)

    hf.summary_res(g_results, sm_results, "first")

    hf.runtime_ratio(t1, t2)


# ============================================================
# 2. Nonlinear curve (linear in parameters)
# ============================================================

def example_nonlinear():
    hf.printhead("OLS nonlinear curve (linear in parameters)")

    nsample = 50
    x = np.linspace(0, 20, nsample)

    X = np.column_stack((
        x,
        np.sin(x),
        (x - 5) ** 2,
        np.ones(nsample)
    ))

    beta = np.array([0.5, 0.5, -0.02, 5.0])

    y_true = X @ beta
    y = y_true + 0.5 * np.random.normal(size=nsample)

    # ---------------- statsmodels ----------------
    if SHOW_STATSMODELS:
        t1 = hf.benchmark_fit(lambda: sm.OLS(y, X).fit())

        sm_results = sm.OLS(y, X).fit()

        print(sm_results.summary())

        hf.runtime(t1)

    # ---------------- gretl ----------------
    hf.linenr(8)

    data = gretl.get_data(
        gretl.nparray2dict(X, Y=y, with_const=True, const_pos=4)
    )

    t2 = hf.benchmark_fit(lambda: gretl.ols("y ~ const + x1 + x2 + x3", data=data).fit())

    g_results = gretl.ols("y ~ const + x1 + x2 + x3", data=data).fit()

    print(g_results)
    #print("yhat:\n", g_results.yhat)

    hf.runtime(t2)

    hf.summary_res(g_results, sm_results, "last")

    hf.runtime_ratio(t1, t2)


# ============================================================
# 3. Dummy variables
# ============================================================

def example_dummy_variables():
    hf.printhead("OLS with dummy variables")

    nsample = 50

    groups = np.zeros(nsample, dtype=int)
    groups[20:40] = 1
    groups[40:] = 2

    dummy = pd.get_dummies(groups).values

    x = np.linspace(0, 20, nsample)

    X = np.column_stack((x, dummy[:, 1:]))
    X = sm.add_constant(X, prepend=False)

    beta = np.array([1.0, 3.0, -3.0, 10.0])

    y_true = X @ beta
    y = y_true + np.random.normal(size=nsample)

    # ---------------- statsmodels ----------------
    if SHOW_STATSMODELS:
        t0 = time.perf_counter()

        sm_results = sm.OLS(y, X).fit()

        t1 = time.perf_counter()

        print(sm_results.summary())

        pred = sm_results.get_prediction().summary_frame()

        iv_l = pred["obs_ci_lower"]
        iv_u = pred["obs_ci_upper"]

        plt.figure(figsize=(8, 6))
        plt.plot(x, y, "o")
        plt.plot(x, y_true, "b-")
        plt.plot(x, sm_results.fittedvalues, "r--.")
        plt.plot(x, iv_l, "r--")
        plt.plot(x, iv_u, "r--")
        plt.title('statsmodels')

        hf.runtime(t0, t1)

    # ---------------- gretl ----------------
    hf.linenr(13)

    data = gretl.get_data(
        gretl.nparray2dict(X, Y=y, with_const=True, const_pos=3)
    )

    t0 = time.perf_counter()

    g_results = gretl.ols("y ~ const + x1 + x2 + x3", data=data).fit()

    t1 = time.perf_counter()

    print(g_results)

    iv_l = g_results.yhat - 2.0 * g_results.sigma
    iv_u = g_results.yhat + 2.0 * g_results.sigma

    plt.figure(figsize=(8, 6))
    plt.plot(x, y, "o")
    plt.plot(x, y_true, "b-")
    plt.plot(x, g_results.yhat, "r--.")
    plt.plot(x, iv_l, "r--")
    plt.plot(x, iv_u, "r--")
    plt.title('gretl4py')

    hf.runtime(t0, t1)
    plt.show()


# ============================================================
# 4. Multicollinearity (Longley)
# ============================================================

def example_multicollinearity():
    hf.printhead("Multicollinearity example (Longley)")

    from statsmodels.datasets.longley import load_pandas

    data = load_pandas()
    y = data.endog
    X = sm.add_constant(data.exog)

    # ---------------- statsmodels ----------------
    if SHOW_STATSMODELS:
        t0 = time.perf_counter()

        sm_results = sm.OLS(y, X).fit()

        t1 = time.perf_counter()

        print(sm_results.summary())

        hf.runtime(t0, t1)

    # ---------------- gretl ----------------
    hf.linenr(21)

    df = pd.concat([X, y], axis=1)
    d4 = gretl.get_data(gretl.df2dict(df, 1))
    d4.set_as_default()

    t0 = time.perf_counter()

    g_results = gretl.ols(
        "TOTEMP ~ const + GNPDEFL + GNP + UNEMP + ARMED + POP + YEAR"
    ).fit()

    t1 = time.perf_counter()

    print(g_results)

    hf.runtime(t0, t1)


# ============================================================
# 5. Dropping an observation
# ============================================================

def example_drop_observation():
    hf.printhead("Dropping an observation")

    from statsmodels.datasets.longley import load_pandas

    data = load_pandas()
    y = data.endog
    X = sm.add_constant(data.exog)

    sm_full = sm.OLS(y, X).fit()
    sm_reduced = sm.OLS(y.iloc[:14], X.iloc[:14]).fit()

    if SHOW_STATSMODELS:
        print(
            "Parameter change (%):\n",
            (sm_reduced.params - sm_full.params) / sm_full.params * 100
        )

    df = pd.concat([X, y], axis=1)
    d4 = gretl.get_data(gretl.df2dict(df, 1))

    d4.sample(1, 14)

    g_full = gretl.ols(
        "TOTEMP ~ const + GNPDEFL + GNP + UNEMP + ARMED + POP + YEAR"
    ).fit()

    d4.sample(1, 14)

    g_reduced = gretl.ols(
        "TOTEMP ~ const + GNPDEFL + GNP + UNEMP + ARMED + POP + YEAR"
    ).fit()

    print(
        "Parameter change (%):\n",
        [
            100 * (a - b) / b
            for a, b in zip(g_reduced.coeff.numpy_view,
                            g_full.coeff.numpy_view)
        ]
    )


# ============================================================
# main
# ============================================================

def main():
    #example_ols()
    example_nonlinear()
    #example_dummy_variables()
    #example_multicollinearity()
    #example_drop_observation()


if __name__ == "__main__":
    main()
