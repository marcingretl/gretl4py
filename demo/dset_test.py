import sys
sys.path.append("../")
import gretl
import numpy
import pandas
from gretl4py_addons import gretl_model_print
from gretl4py_addons import csv2dict
from gretl4py_addons import df2dict

DSET = csv2dict('data9-7.csv', 4)

print("Loading global dataset:")
gretl.load_data(DSET)

print("\nOLS example using global dataset:\n")
res = gretl.ols('QNC ~ 0 + PRICE + INCOME', print=True)
gretl_model_print(res)

print("\nOLS example passing dataset as dict:\n")
res = gretl.ols('QNC ~ 0 + PRICE + INCOME', DSET, print=True)
gretl_model_print(res)

dframe = pandas.read_csv('data9-7.csv')
MyDict = df2dict(dframe)
res = gretl.ols('QNC ~ 0 + PRICE + INCOME', MyDict, print=True)
gretl_model_print(res)
