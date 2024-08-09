import gretl
import numpy as np
from gretl4py_addons import gretl_data_path
from gretl4py_classes import *

fname = gretl_data_path('ramanathan/data9-7.gdt')
data97 = gretl.get_gretl_data(fname)

OLS1 = gretl.ols('QNC ~ const + PRICE + INCOME', data=data97).fit()
print(OLS1.args)
print(OLS1) # print(OLS1.summary) works as well
print(OLS1.results['vcv'])
print(f'\nIs Model {OLS1.id():d} asymptotic?: {OLS1.results["asymptotic"]}')
