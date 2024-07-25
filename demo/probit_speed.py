import sys
sys.path.append("../")
import time

import gretl
import pandas
from gretl4py_addons import df2dict

Ptest = pandas.read_csv('large_probit.csv')
MyDict = df2dict(Ptest)

reglist = open('prob_frml.txt', 'r').read()

# probit model via gretl
frml = 'y ~ const+' + reglist[0:-1]
t0 = time.time()
gmod = gretl.probit(frml, data=MyDict)
t1 = time.time() - t0
print(gmod["coeff"])

# probit model via statsmodels
from statsmodels.discrete.discrete_model import Probit
frml = 'y ~ ' + reglist[0:-1]
t0 = time.time()
smod = Probit.from_formula(frml, Ptest).fit()
t2 = time.time() - t0
print(smod.params)

print(f"\ngretl = {t1} seconds, statsmodels = {t2} seconds\n")
print(f"ratio = {t1/t2}\n")
