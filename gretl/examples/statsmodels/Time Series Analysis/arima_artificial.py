import numpy as np
import pandas as pd
from statsmodels.tsa.arima_process import arma_generate_sample
from statsmodels.tsa.arima.model import ARIMA

import gretl
import sys
import time

sys.path.append('../')
import helper_func as hf


np.random.seed(12345)


"""
Here, we have statsmodels ARMA examples available at

https://www.statsmodels.org/dev/examples/notebooks/generated/tsa_arma_1.html#,

rendered as gretl calls.
"""


Do_StatsModels = True


def main():

    hf.printhead("Autoregressive Moving Average (ARMA): Artificial data")


    # ============================================================
    # data generation
    # ============================================================

    arparams = np.array([0.75, -0.25])
    maparams = np.array([0.65, 0.35])

    arparams = np.r_[1, -arparams]
    maparams = np.r_[1, maparams]

    nobs = 250

    y = arma_generate_sample(arparams, maparams, nobs)

    dates = pd.date_range(
        "1980-01-01",
        periods=nobs,
        freq="ME"
    )

    y = pd.Series(y, index=dates, name="y")

    # data generation: END


    # ============================================================
    # statsmodels
    # ============================================================

    if Do_StatsModels:

        hf.linenr(1)

        t0 = time.perf_counter()

        sm_model = ARIMA(
            y,
            order=(2, 0, 2),
            trend="n"
        )

        sm_results = sm_model.fit()

        t1 = time.perf_counter()

        sm_time = t1 - t0

        print(sm_results.summary())

        print("\nParameters:")
        print(sm_results.params)

        hf.runtime(sm_time)


    # ============================================================
    # gretl4py
    # ============================================================

    hf.linenr(2)

    d1 = gretl.get_data(
        gretl.series2dict(y, 12)
    )


    t0 = time.perf_counter()

    gr_model = gretl.arima(
        y="y",
        order=(2, 0, 2),
        nc=True,
        opg=True,
        data=d1
    )

    gr_results = gr_model.fit()

    t1 = time.perf_counter()

    gr_time = t1 - t0


    print(gr_results)

    hf.runtime(gr_time)


    # ============================================================
    # comparison
    # ============================================================

    hf.summary_res(
    gr_results,
    sm_results,
    drop_last=True
    )

    hf.runtime_ratio(
        sm_time,
        gr_time
    )


if __name__ == "__main__":
    main()
