import matplotlib.pyplot as plt
#import numpy as np
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
Here, we have statsmodels ARMA examples available at\n\
https://www.statsmodels.org/dev/examples/notebooks/generated/tsa_arma_0.html,\n\
rendered as gretl calls.
"""

Do_StatsModels = True # if we want to see statsmodels results as well

"""
################################################################
***     Autoregressive Moving Average (ARMA): Sunspots data  ***
################################################################
"""
hf.printhead("Sunspots Data")
print(sm.datasets.sunspots.NOTE)

# data
dta = sm.datasets.sunspots.load_pandas().data
dta.index = pd.Index(sm.tsa.datetools.dates_from_range("1700", "2008"))
dta.index.freq = dta.index.inferred_freq
del dta["YEAR"]
# data: END

# plots
dta.plot(figsize=(12, 8))
fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(211)
fig = sm.graphics.tsa.plot_acf(dta.values.squeeze(), lags=40, ax=ax1)
ax2 = fig.add_subplot(212)
fig = sm.graphics.tsa.plot_pacf(dta, lags=40, ax=ax2)
# plots: END

# statsmodels calls:
if Do_StatsModels is True:
    t0 = time.time()
    arma_mod20 = ARIMA(dta, order=(2, 0, 0)).fit()
    t1 = time.time()
    print(arma_mod20.params)
    hf.runtime(t0, t1)
    t0 = time.time()
    arma_mod30 = ARIMA(dta, order=(3, 0, 0)).fit()
    t1 = time.time()
    print(arma_mod20.aic, arma_mod20.bic, arma_mod20.hqic)
    sm.stats.durbin_watson(arma_mod30.resid.values)
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111)
    ax = arma_mod30.resid.plot(ax=ax)
    resid = arma_mod30.resid
    stats.normaltest(resid)
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111)
    fig = qqplot(resid, line="q", ax=ax, fit=True)
    hf.runtime(t0, t1)

# gretl calls
gr_dta = gretl.get_data(gretl.df2dict(dta, 1))
t0 = time.time()
arma_mod20 = gretl.arima(y='SUNACTIVITY', order=(2, 0, 0), data=gr_dta).fit()
t1 = time.time()
hf.linenr(9)
print(arma_mod20.parnames, arma_mod20.coeff)
print("sigma2: " + str(arma_mod20.sigma**2))
hf.runtime(t0, t1)
t0 = time.time()
arma_mod30 = gretl.arima(y='SUNACTIVITY', order=(3, 0, 0), data=gr_dta).fit()
t1 = time.time()
hf.linenr(11)
print(arma_mod20.aic, arma_mod20.bic, arma_mod20.hqc)
hf.linenr(12)
print(arma_mod30.parnames, arma_mod30.coeff)
print("sigma2: " + str(arma_mod30.sigma**2))
hf.linenr(13)
print(arma_mod30.aic, arma_mod30.bic, arma_mod30.hqc)
resid = arma_mod30.uhat
hf.linenr(17)
print(stats.normaltest(resid))
hf.linenr(18)
fig = plt.figure(figsize=(12, 8))
ax = fig.add_subplot(111)
fig = qqplot(pd.Series(resid), line="q", ax=ax, fit=True)
hf.runtime(t0, t1)
