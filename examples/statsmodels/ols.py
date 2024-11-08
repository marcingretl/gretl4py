#import matplotlib.pyplot as plt
import numpy as np
#import pandas as pd
import statsmodels.api as sm
import gretl

np.random.seed(9876789)

def printhead (title):
    hashedline = "################################################################"
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
        print("an exception must be raised!!!")
    if Y is not None:
        if not isinstance(Y, numpy.ndarray):
            print("an exception must be raised!!!")
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
"""
model = sm.OLS(y, X_sm)
results = model.fit()
print(results.summary())
print("Parameters: ", results.params)
print("R2: ", results.rsquared)
"""

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
"""
res = sm.OLS(y, X).fit()
print(res.summary())
print("Parameters: ", res.params)
print("Standard errors: ", res.bse)
print("Predicted values: ", res.predict())
"""

# gretl calls
res = gretl.ols('y ~ const + x1 + x2 + x3', data=numpyarray2dict(X, Y=y, with_const=True, const_pos=4)).fit()
print(res)
print("Parameters: ", res.params)
print("Standard errors: ", res.bse)
print("Predicted values: ", None)
