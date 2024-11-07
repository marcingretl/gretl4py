#import matplotlib.pyplot as plt
import numpy as np
#import pandas as pd
import statsmodels.api as sm
import gretl

np.random.seed(9876789)
hashedline = "################################################################"

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


""" OLS estimation """
print(f'\n{hashedline:s}\n***\tOLS estimation\t***\n{hashedline:s}')
nsample = 100
x = np.linspace(0, 10, 100)
X = np.column_stack((x, x ** 2))
beta = np.array([1, 0.1, 10])
e = np.random.normal(size=nsample)

X_sm = sm.add_constant(X)
y = np.dot(X_sm, beta) + e

gretl.attach_data(numpyarray2dict(X, Y=y, with_const=False))
#gretl.attach_data(numpyarray2dict(X_sm, Y=y, with_const=True)) # this is alternative call

model = gretl.ols('y ~ const + x1 + x2')
results = model.fit()
print(results)
print("Parameters: ", np.transpose(model.results['coeff']))
print("R2: ", model.results['rsq'])


""" OLS non-linear curve but linear in parameters """
print(f'\n{hashedline:s}\n***\tOLS non-linear curve but linear in parameters\t***\n{hashedline:s}')
nsample = 50
sig = 0.5
x = np.linspace(0, 20, nsample)
X = np.column_stack((x, np.sin(x), (x - 5) ** 2, np.ones(nsample)))
beta = [0.5, 0.5, -0.02, 5.0]

y_true = np.dot(X, beta)
y = y_true + sig * np.random.normal(size=nsample)

gretl.attach_data(numpyarray2dict(X, Y=y, with_const=True, const_pos=4))

res = gretl.ols('y ~ const + x1 + x2 + x3').fit()
print(res)
print("Parameters: ", np.transpose(res.results['coeff']))
print("Standard errors: ", np.transpose(res.results['stderr']))
print("Predicted values: ", None)
