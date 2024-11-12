import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import statsmodels.api as sm
import gretl

np.random.seed(9876789)

def printhead (title, simple=False):
    hashedline = "################################################################"
    if simple is True:
        print(f'\n***\t{title:s}\t***')
    else:
        print(f'\n{hashedline:s}\n***\t{title:s}\t***\n{hashedline:s}')

def numpyarray2dict (X, periodicity=1, Y=None, with_const=False,
                     const_pos=1):    
    '''turns a 'raw' numpy array into a dictionary gretl understands
    X - matrix with observations on explanatory variables
    Y - vector with observations on endogeneus variable
    with_const - boolean indicating if X contains constant term
    const_pos - column number which holds constant term values
    '''
    import numpy
    
    if not isinstance(X, numpy.ndarray):
        print("X is not a numpay array!!!")
    if Y is not None:
        if not isinstance(Y, numpy.ndarray):
            print("Y is not a numpay array!!!")
        if numpy.shape(Y)[0] != numpy.shape(X)[0]:
            print("an exception must be raised!!!")

    d = dict()
    obs_labels = list()
    varname = list()

    if Y is not None:
        # force Tx1 vector
        Y = numpy.vstack(Y)
        varname.append("y")

    # we create artificial variables names
    #if with_const:
    #    varname.append("constant")
   
    for i in range(with_const, numpy.shape(X)[1]):
        varname.append("x" + str(i+1-with_const)) 
    
    if with_const:
        varname.insert(const_pos, "constant")
        
    # we create artificial obs labels
    for i in range(numpy.shape(X)[0]):
        obs_labels.append(i+1)
    
    d['data_array'] = numpy.hstack((Y, X))
    d['varname'] = varname
    d['obs_labels'] = obs_labels
    d['periodicity'] = periodicity
    
    return d

"""
Here, we have statsmodels OLS examples available at\n\
https://www.statsmodels.org/stable/examples/notebooks/generated/ols.html,\n\
rendered as gretl calls.
"""

Do_StatsModels = False # if we want to see statsmodels results as well

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
model = gretl.ols('y ~ const + x1 + x2', data=numpyarray2dict(X, Y=y, with_const=False))
#model = gretl.ols('y ~ const + x1 + x2', data=numpyarray2dict(X_sm, Y=y, with_const=True)) # this is an alternative call using X_sm
results = model.fit()
print(results.summary)
print("Parameters: ", results.params)
print("R2: ", results.rsquared)
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
res = gretl.ols('y ~ const + x1 + x2 + x3', data=numpyarray2dict(X, Y=y, with_const=True, const_pos=4)).fit()
print(res)
print("Parameters: ", res.params)
print("Standard errors: ", res.bse)
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
res = gretl.ols('y ~ const + x1 + x2 + x3', data=numpyarray2dict(X, Y=y, with_const=True, const_pos=4)).fit()
print(res)
#pred_ols = res.get_prediction()
#iv_l = pred_ols.summary_frame()["obs_ci_lower"]
#iv_u = pred_ols.summary_frame()["obs_ci_upper"]
fig, ax = plt.subplots(figsize=(8, 6))
ax.plot(x, y, "o", label="Data")
ax.plot(x, y_true, "b-", label="True")
ax.plot(x, res.fittedvalues, "r--.", label="Predicted")
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
    ols_model = sm.OLS(y, X)
    ols_results = ols_model.fit()
    print(ols_results.summary())

# gretl calls
df = pd.concat([X, y], axis=1) # let's try to use X.iloc[:, 1:7] (const term issue)
gretl.attach_data(gretl.df2dict(df, 1))
ols_model = gretl.ols('TOTEMP ~ const + GNPDEFL + GNP + UNEMP + ARMED + POP + YEAR')
ols_results = ols_model.fit()
print(ols_results)

printhead("Condition number - to be done", True)
    
printhead("Dropping an observation", True)
# statsmodels calls:
if Do_StatsModels is True:
    ols_results2 = sm.OLS(y.iloc[:14], X.iloc[:14]).fit()
    print(
        "Percentage change %4.2f%%\n"
        * 7
        % tuple(
            [
                i
                for i in (ols_results2.params - ols_results.params)
                / ols_results.params
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
            for i in (ols_results2.params - ols_results.params)
            / ols_results.params
            * 100
        ]
    )
)
