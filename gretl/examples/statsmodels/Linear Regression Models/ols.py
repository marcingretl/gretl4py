import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import statsmodels.api as sm
import gretl
import sys
import time
sys.path.append('../')
import helper_func as hf

np.random.seed(9876789)

"""
Here, we have statsmodels OLS examples available at\n\
https://www.statsmodels.org/stable/examples/notebooks/generated/ols.html,\n\
rendered as gretl calls.
"""

Do_StatsModels = True # if we want to see statsmodels results as well

"""
################################################################
***     OLS estimation  ***
################################################################
"""
hf.printhead("OLS estimation")
# data generation (package independent)
nsample = 100
x = np.linspace(0, 10, 100)
X = np.column_stack((x, x ** 2))
beta = np.array([1, 0.1, 10])
e = np.random.normal(size=nsample)
X_sm = sm.add_constant(X)
y = np.dot(X_sm, beta) + e
# data generation: END

# statsmodels calls:
if Do_StatsModels is True:
    t0 = time.time()
    model = sm.OLS(y, X_sm)
    results = model.fit()
    t1 = time.time()
    print(results.summary())
    print("Parameters: ", results.params)
    print("R2: ", results.rsquared)
    hf.runtime(t0, t1)

# gretl calls
hf.linenr(5)
d1 = gretl.get_data(gretl.nparray2dict(X, Y=y, with_const=False))
t0 = time.time()
model = gretl.ols('y ~ const + x1 + x2', data=d1)
results = model.fit()
t1 = time.time()
print(results.summary)
hf.linenr(6)
print("Parameters: ", results.coeff)
print("R2: ", results.rsq)
hf.runtime(t0, t1)
# OLS estimation: END

"""
################################################################
***     OLS non-linear curve but linear in parameters   ***
################################################################
"""
hf.printhead("OLS non-linear curve but linear in parameters")
# data generation (package independent)
nsample = 50
sig = 0.5
x = np.linspace(0, 20, nsample)
X = np.column_stack((x, np.sin(x), (x - 5) ** 2, np.ones(nsample)))
beta = [0.5, 0.5, -0.02, 5.0]
y_true = np.dot(X, beta)
y = y_true + sig * np.random.normal(size=nsample)
# data generation: END

# statsmodels calls:
if Do_StatsModels is True:
    t0 = time.time()
    res = sm.OLS(y, X).fit()
    t1 = time.time()
    print(res.summary())
    print("Parameters: ", res.params)
    print("Standard errors: ", res.bse)
    print("Predicted values: ", res.predict())
    hf.runtime(t0, t1)

# gretl calls
hf.linenr(8)
t0 = time.time()
d2 = gretl.get_data(gretl.nparray2dict(X, Y=y,
                                        with_const=True, const_pos=4))
res = gretl.ols('y ~ const + x1 + x2 + x3', data=d2).fit()
t1 = time.time()
print(res)
hf.linenr(9)
print("Parameters: ", res.coeff)
print("Standard errors: ", res.stderr)
print("Predicted values: ", res.yhat)
hf.runtime(t0, t1)
# OLS non-linear curve but linear in parameters: END

"""
################################################################
***     OLS with dummy variables   ***
################################################################
"""
hf.printhead("OLS with dummy variables")
# data generation (package independent)
nsample = 50
groups = np.zeros(nsample, int)
groups[20:40] = 1
groups[40:] = 2
# dummy = (groups[:,None] == np.unique(groups)).astype(float)
dummy = pd.get_dummies(groups).values
x = np.linspace(0, 20, nsample)
# drop reference category
X = np.column_stack((x, dummy[:, 1:]))
X = sm.add_constant(X, prepend=False)

beta = [1.0, 3, -3, 10]
y_true = np.dot(X, beta)
e = np.random.normal(size=nsample)
y = y_true + e

print(X[:5, :])
print(y[:5])
print(groups)
print(dummy[:5, :])
# data generation: END

# statsmodels calls:
if Do_StatsModels is True:
    t0 = time.time()
    res2 = sm.OLS(y, X).fit()
    t1 = time.time()
    print(res2.summary())
    pred_ols2 = res2.get_prediction()
    iv_l = pred_ols2.summary_frame()["obs_ci_lower"]
    iv_u = pred_ols2.summary_frame()["obs_ci_upper"]
    fig, ax = plt.subplots(figsize=(8, 6))
    ax.plot(x, y, "o", label="Data")
    ax.plot(x, y_true, "b-", label="True")
    ax.plot(x, res2.fittedvalues, "r--.", label="Predicted")
    ax.plot(x, iv_u, "r--")
    ax.plot(x, iv_l, "r--")
    legend = ax.legend(loc="best")
    hf.runtime(t0, t1)

# gretl calls
hf.linenr(13)
d3 = gretl.get_data(gretl.nparray2dict(X, Y=y,
                                        with_const=True, const_pos=4))
t0 = time.time()
res = gretl.ols('y ~ const + x1 + x2 + x3', data=d3).fit()
t1 = time.time()
print(res)
hf.linenr(14)
iv_l = res.yhat - 2.0129 * res.sigma
iv_u = res.yhat + 2.0129 * res.sigma
fig, ax = plt.subplots(figsize=(8, 6))
ax.plot(x, y, "o", label="Data")
ax.plot(x, y_true, "b-", label="True")
ax.plot(x, res.yhat, "r--.", label="Predicted")
ax.plot(x, iv_u, "r--")
ax.plot(x, iv_l, "r--")
legend = ax.legend(loc="best")
hf.runtime(t0, t1)
plt.show()
# OLS with dummy variables: END

"""
################################################################
***     Joint hypothesis test - Multicollinearity  ***
################################################################
"""
hf.printhead("Joint hypothesis test - Multicollinearity")
"""
Note, that longley dataset is available in standard gretl distribution:
fname = gretl.gretl_data_path("misc/longley.gdt")
longley = gretl.get_data(fname)
"""
from statsmodels.datasets.longley import load_pandas
y = load_pandas().endog
X = load_pandas().exog
X = sm.add_constant(X)

# statsmodels calls:
if Do_StatsModels is True:
    t0 = time.time()
    sm_ols_model = sm.OLS(y, X)
    sm_ols_results = sm_ols_model.fit()
    t1 = time.time()
    print(sm_ols_results.summary())
    hf.runtime(t0, t1)

# gretl calls
hf.linenr(21)
df = pd.concat([X, y], axis=1) # could it be X.iloc[:, 1:7]
d4 = gretl.get_data(gretl.df2dict(df, 1))
d4.set_as_default()
t0 = time.time()
ols_model = gretl.ols('TOTEMP ~ const + GNPDEFL + GNP + UNEMP + ARMED + POP + YEAR')
ols_results = ols_model.fit()
t1 = time.time()
print(ols_results)
hf.runtime(t0, t1)

hf.printhead("Dropping an observation")
# statsmodels calls:
if Do_StatsModels is True:
    t0 = time.time()
    sm_ols_results2 = sm.OLS(y.iloc[:14], X.iloc[:14]).fit()
    t1 = time.time()
    print(
        "Percentage change %4.2f%%\n"
        * 7
        % tuple(
            [
                i
                for i in (sm_ols_results2.params - sm_ols_results.params)
                / sm_ols_results.params
                * 100
            ]
        )
    )
    hf.runtime(t0, t1)

# gretl calls
hf.linenr(24)
t0 = time.time()
d4.sample(t1=1, t2=14)
ols_results2 = gretl.ols('TOTEMP ~ const + GNPDEFL + GNP + UNEMP + ARMED + POP + YEAR').fit()
t1 = time.time()
print(
    "Percentage change %4.2f%%\n"
    * 7
    % tuple(
        [
            i
            for i in [100*(a - b)/b for a, b in zip(ols_results2.coeff, ols_results.coeff)]
        ]
    )
)
hf.runtime(t0, t1)
