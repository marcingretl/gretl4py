import gretl
import time
import pandas

Ptest = pandas.read_csv('large_probit.csv')
MyData = gretl.get_data('large_probit.csv')

reglist = open('prob_frml.txt', 'r').read()

res = gretl.Matrix(25,2)

# probit model via gretl
frml = 'y ~ const+' + reglist[0:-1]
t0 = time.time()
gmod = gretl.probit(frml, data=MyData).fit()
t1 = time.time() - t0
res.numpy_view[:, 0] = gmod.coeff.numpy_view

# probit model via statsmodels
from statsmodels.discrete.discrete_model import Probit
frml = 'y ~ ' + reglist[0:-1]
t0 = time.time()
smod = Probit.from_formula(frml, Ptest).fit()
t2 = time.time() - t0
res.numpy_view[:, 1] = smod.params

res.rnameset(gmod.parnames)
res.cnameset(['gretl4py', 'statsmodels'])
print(f'\n{res}')

print(f"\ngretl = {t1} seconds, statsmodels = {t2} seconds\n")
print(f"ratio = {t1/t2}\n")
