import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import statsmodels.api as sm
import gretl

np.random.seed(9876789)

def printhead (title, full=True):
    hashedline = "################################################################"
    if full is False:
        print(f'\n***\t{title:s}\t***')
    else:
        print(f'\n{hashedline:s}\n***\t{title:s}\t***\n{hashedline:s}')

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
printhead("OLS estimation")
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
    model = sm.OLS(y, X_sm)
    results = model.fit()
    print(results.summary())
    print("Parameters: ", results.params)
    print("R2: ", results.rsquared)

# gretl calls
model = gretl.ols('y ~ const + x1 + x2',
                  data=gretl.numpyarray2dict(X, Y=y, with_const=False))
#model = gretl.ols('y ~ const + x1 + x2', data=gretl.numpyarray2dict(X_sm, Y=y, with_const=True)) # this is an alternative call using X_sm
results = model.fit()
print(results.summary)
print("Parameters: ", results.coeff())
print("R2: ", results.rsq())
# OLS estimation: END

"""
################################################################
***     OLS non-linear curve but linear in parameters   ***
################################################################
"""
printhead("OLS non-linear curve but linear in parameters")
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
    res = sm.OLS(y, X).fit()
    print(res.summary())
    print("Parameters: ", res.params)
    print("Standard errors: ", res.bse)
    print("Predicted values: ", res.predict())

# gretl calls
res = gretl.ols('y ~ const + x1 + x2 + x3',
                data=gretl.numpyarray2dict(X, Y=y,
                                           with_const=True, const_pos=4)).fit()
print(res)
print("Parameters: ", res.coeff())
print("Standard errors: ", res.stderr())
print("Predicted values: ", None)
# OLS non-linear curve but linear in parameters: END

"""
################################################################
***     OLS with dummy variables   ***
################################################################
"""
printhead("OLS with dummy variables")
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
    res2 = sm.OLS(y, X).fit()
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

# gretl calls
res = gretl.ols('y ~ const + x1 + x2 + x3',
                data=gretl.numpyarray2dict(X, Y=y,
                                           with_const=True, const_pos=4)).fit()
print(res)
#pred_ols = res.get_prediction()
#iv_l = pred_ols.summary_frame()["obs_ci_lower"]
#iv_u = pred_ols.summary_frame()["obs_ci_upper"]
fig, ax = plt.subplots(figsize=(8, 6))
ax.plot(x, y, "o", label="Data")
ax.plot(x, y_true, "b-", label="True")
ax.plot(x, res.yhat(), "r--.", label="Predicted")
#ax.plot(x, iv_u, "r--")
#ax.plot(x, iv_l, "r--")
legend = ax.legend(loc="best")
plt.show()
# OLS with dummy variables: END

"""
################################################################
***     Joint hypothesis test - Multicollinearity  ***
################################################################
"""
printhead("Joint hypothesis test - Multicollinearity")
"""
Note, that longley dataset is available in standard gretl distribution:
fname = gretl.gretl_data_path("misc/longley.gdt")
longley = gretl.get_gretl_data(fname)
"""
from statsmodels.datasets.longley import load_pandas
y = load_pandas().endog
X = load_pandas().exog
X = sm.add_constant(X)

# statsmodels calls:
if Do_StatsModels is True:
    sm_ols_model = sm.OLS(y, X)
    sm_ols_results = sm_ols_model.fit()
    print(sm_ols_results.summary())

# gretl calls
df = pd.concat([X, y], axis=1) # could it be X.iloc[:, 1:7]
gretl.attach_data(gretl.df2dict(df, 1))
ols_model = gretl.ols('TOTEMP ~ const + GNPDEFL + GNP + UNEMP + ARMED + POP + YEAR')
ols_results = ols_model.fit()
print(ols_results)
   
printhead("Dropping an observation")
# statsmodels calls:
if Do_StatsModels is True:
    sm_ols_results2 = sm.OLS(y.iloc[:14], X.iloc[:14]).fit()
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

# gretl calls
gretl.sample(t1=1, t2=14)
ols_results2 = gretl.ols('TOTEMP ~ const + GNPDEFL + GNP + UNEMP + ARMED + POP + YEAR').fit()
print(
    "Percentage change %4.2f%%\n"
    * 7
    % tuple(
        [
            i
            for i in (ols_results2.coeff() - ols_results.coeff())
            / ols_results.coeff()
            * 100
        ]
    )
)
