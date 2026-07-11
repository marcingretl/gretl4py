import matplotlib.pyplot as plt
import pandas as pd
import statsmodels.api as sm
from scipy import stats
from statsmodels.tsa.arima.model import ARIMA
from statsmodels.graphics.api import qqplot

import gretl
import sys
import time

sys.path.append('../')
import helper_func as hf


"""
Here, we have statsmodels ARMA examples available at:
https://www.statsmodels.org/dev/examples/notebooks/generated/tsa_arma_0.html

rendered as gretl calls.
"""

Do_StatsModels = True


def main():

    hf.printhead("Autoregressive Moving Average (ARMA): Sunspots data")

    print(sm.datasets.sunspots.NOTE)

    # ------------------------------------------------------------
    # Data
    # ------------------------------------------------------------
    dta = sm.datasets.sunspots.load_pandas().data
    dta.index = pd.Index(
        sm.tsa.datetools.dates_from_range("1700", "2008")
    )
    dta.index.freq = dta.index.inferred_freq
    del dta["YEAR"]


    # ------------------------------------------------------------
    # Plots
    # ------------------------------------------------------------
    dta.plot(figsize=(12, 8))

    fig = plt.figure(figsize=(12, 8))

    ax1 = fig.add_subplot(211)
    sm.graphics.tsa.plot_acf(
        dta.values.squeeze(),
        lags=40,
        ax=ax1
    )

    ax2 = fig.add_subplot(212)
    sm.graphics.tsa.plot_pacf(
        dta,
        lags=40,
        ax=ax2
    )


    # ------------------------------------------------------------
    # ARMA(2,0)
    # ------------------------------------------------------------
    hf.printhead("ARMA(2,0)", full=False)

    if Do_StatsModels:

        t0 = time.perf_counter()

        sm_arma20 = ARIMA(
            dta,
            order=(2, 0, 0)
        ).fit()

        sm_time20 = time.perf_counter() - t0

        print(sm_arma20.summary())
        print(sm_arma20.params)

        hf.runtime(sm_time20)


    gr_dta = gretl.get_data(
        gretl.df2dict(dta, 1)
    )

    t0 = time.perf_counter()

    gr_arma20 = gretl.arima(
        y="SUNACTIVITY",
        order=(2, 0, 0),
        data=gr_dta
    ).fit()

    gr_time20 = time.perf_counter() - t0

    hf.linenr(1)

    print(gr_arma20)
    print(gr_arma20.parnames)
    print(gr_arma20.coeff)

    print("sigma2:", gr_arma20.sigma ** 2)

    hf.runtime(gr_time20)

    if Do_StatsModels:
        hf.summary_res(
            gr_arma20,
            sm_arma20,
            drop_last=True
        )

        hf.runtime_ratio(
            sm_time20,
            gr_time20
        )


    # ------------------------------------------------------------
    # ARMA(3,0)
    # ------------------------------------------------------------
    hf.printhead("ARMA(3,0)", full=False)

    if Do_StatsModels:

        t0 = time.perf_counter()

        sm_arma30 = ARIMA(
            dta,
            order=(3, 0, 0)
        ).fit()

        sm_time30 = time.perf_counter() - t0

        print(sm_arma30.summary())

        print(
            "AIC:",
            sm_arma30.aic,
            "BIC:",
            sm_arma30.bic,
            "HQIC:",
            sm_arma30.hqic
        )

        hf.runtime(sm_time30)


    t0 = time.perf_counter()

    gr_arma30 = gretl.arima(
        y="SUNACTIVITY",
        order=(3, 0, 0),
        data=gr_dta
    ).fit()

    gr_time30 = time.perf_counter() - t0


    hf.linenr(2)

    print(gr_arma30)
    print(gr_arma30.parnames)
    print(gr_arma30.coeff)

    print("sigma2:", gr_arma30.sigma ** 2)

    print(
        "AIC:",
        gr_arma30.aic,
        "BIC:",
        gr_arma30.bic,
        "HQIC:",
        gr_arma30.hqc
    )

    hf.runtime(gr_time30)


    if Do_StatsModels:

        hf.summary_res(
            gr_arma30,
            sm_arma30,
            drop_last=True
        )

        hf.runtime_ratio(
            sm_time30,
            gr_time30
        )


    # ------------------------------------------------------------
    # Residual diagnostics
    # ------------------------------------------------------------

    resid = gr_arma30.uhat

    hf.printhead("Residual diagnostics", full=False)

    print(stats.normaltest(resid))

    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111)

    qqplot(
        pd.Series(resid),
        line="q",
        ax=ax,
        fit=True
    )

    plt.show()


if __name__ == "__main__":
    main()
