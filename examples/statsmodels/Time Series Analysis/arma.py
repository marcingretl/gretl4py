import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import statsmodels.api as sm
from scipy import stats
from statsmodels.tsa.arima.model import ARIMA
from statsmodels.graphics.api import qqplot
import gretl

np.random.seed(9876789)

def printhead (title, full=True):
    hashedline = "################################################################"
    if full is False:
        print(f'\n***\t{title:s}\t***')
    else:
        print(f'\n{hashedline:s}\n***\t{title:s}\t***\n{hashedline:s}')

def linenr (nr):
    print(f'\n[{nr:d}]:')
    
"""
Here, we have statsmodels OLS examples available at\n\
https://www.statsmodels.org/stable/examples/notebooks/generated/ols.html,\n\
rendered as gretl calls.
"""

Do_StatsModels = True # if we want to see statsmodels results as well

"""
################################################################
***     Autoregressive Moving Average (ARMA)  ***
################################################################
"""
printhead("Sunspots Data")
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
    arma_mod20 = ARIMA(dta, order=(2, 0, 0)).fit()
    print(arma_mod20.params)
    arma_mod30 = ARIMA(dta, order=(3, 0, 0)).fit()
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

# gretl calls
gr_dta = gretl.df2dict(dta, 1)
arma_mod20 = gretl.arma(y='SUNACTIVITY', order=(2, 0, 0), data=gr_dta).fit()
linenr(9)
print(arma_mod20.parnames(), arma_mod20.coeff())
print("sigma2: " + str(arma_mod20.sigma()**2))
arma_mod30 = gretl.arma(y='SUNACTIVITY', order=(3, 0, 0), data=gr_dta).fit()
linenr(11)
print(arma_mod20.aic(), arma_mod20.bic(), arma_mod20.hqc())
linenr(12)
print(arma_mod30.parnames(), arma_mod30.coeff())
print("sigma2: " + str(arma_mod30.sigma()**2))
linenr(13)
print(arma_mod30.aic(), arma_mod30.bic(), arma_mod30.hqc())
resid = arma_mod30.uhat()
linenr(17)
stats.normaltest(resid)
linenr(18)
fig = plt.figure(figsize=(12, 8))
ax = fig.add_subplot(111)
fig = qqplot(resid, line="q", ax=ax, fit=True)
