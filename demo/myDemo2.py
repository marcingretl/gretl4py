import gretl
import numpy as np
from gretl4py_addons import gretl_data_path
from gretl4py_classes import *

fname = gretl_data_path('ramanathan/data9-7.gdt')
data97 = gretl.get_gretl_data(fname)

OLS1 = gretl.ols('QNC ~ const + PRICE + INCOME', data=data97, fit=True)
print(OLS1.args)
# OLS1.fit()
print(OLS1.summary)
print(OLS1.results['vcv'])
