import numpy as np
import pandas as pd
from statsmodels.graphics.tsaplots import plot_predict
from statsmodels.tsa.arima_process import arma_generate_sample
from statsmodels.tsa.arima.model import ARIMA
import gretl
import sys
import time
sys.path.append('../')
import helper_func as hf

np.random.seed(12345)

"""
Here, we have statsmodels ARMA examples available at\n\
https://www.statsmodels.org/dev/examples/notebooks/generated/tsa_arma_1.html#,\n\
rendered as gretl calls.
"""

Do_StatsModels = True # if we want to see statsmodels results as well

"""
################################################################
***     Autoregressive Moving Average (ARMA): Artificial data  ***
################################################################
"""

# data
arparams = np.array([0.75, -0.25])
maparams = np.array([0.65, 0.35])
arparams = np.r_[1, -arparams]
maparams = np.r_[1, maparams]
nobs = 250
y = arma_generate_sample(arparams, maparams, nobs)
dates = pd.date_range("1980-1-1", freq="M", periods=nobs)
y = pd.Series(y, index=dates)
# data: END

# statsmodels calls:
if Do_StatsModels is True:
    t0 = time.time()
    arma_mod = ARIMA(y, order=(2, 0, 2), trend="n")
    arma_res = arma_mod.fit()
    t1 = time.time()
    print(arma_res.summary())
    hf.runtime(t0, t1)

# gretl calls
d1 = gretl.get_data(gretl.series2dict(y, 12))
t0 = time.time()
arma_mod = gretl.arima(y='y', order=(2, 0, 2), nc=True, data=d1)
arma_res = arma_mod.fit()
t1 = time.time()
hf.linenr(6)
print(arma_res.summary)
hf.runtime(t0, t1)
