import sys
sys.path.append("../")
import gretl
from gretl4py_addons import gretl_data_path
from gretl4py_addons import pretty_print

fname = gretl_data_path("ramanathan/data4-1.gdt")
mydata = gretl.get_gretl_data(fname)

print('OLS without options:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata)
pretty_print(res)

print('OLS with robust option:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata, robust=True)
pretty_print(res)

fname = gretl_data_path('greene/greene19_1.gdt')
ldata = gretl.get_gretl_data(fname)

print('logit example:')
res = gretl.logit('GRADE ~ 0 GPA TUCE PSI', data=ldata)
pretty_print(res)
print('probit example:')
res = gretl.probit('GRADE ~ 0 GPA TUCE PSI', data=ldata)
pretty_print(res)

# load dataset using libgretl and set as global DSET
fname = gretl_data_path('misc/kennan.gdt')
gretl.attach_gretl_data(fname)

print('duration: Kennan example variants:\n')
res = gretl.duration('Ti 0 Prod')
pretty_print(res)
res = gretl.duration('Ti 0 Prod', exponential=True)
pretty_print(res)

# replace the global dataset
gretl.attach_gretl_data('bjg.csv')
print('seasonal arma (0 1 2, 0 1 1):')
res = gretl.arma(y='lg', order=(0,1,2), seasonal=(0,1,1), nc=True)
pretty_print(res)
