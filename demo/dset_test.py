import gretl
import numpy
import pandas
from gretl4py_addons import csv2dict, df2dict
from gretl4py_classes import *

d97 = csv2dict('data9-7.csv', 4)

print("Loading global dataset:")
gretl.load_data(d97)

print("\nOLS example using global dataset:\n")
res = gretl.ols('QNC ~ 0 + PRICE + INCOME').fit()
print(res)

print("\nOLS example passing dataset as dict:\n")
res = gretl.ols('QNC ~ 0 + PRICE + INCOME', data=d97).fit()
print(res)

dframe = pandas.read_csv('data9-7.csv')
MyDict = df2dict(dframe)
res = gretl.ols('QNC ~ 0 + PRICE + INCOME', data=MyDict).fit()
print(res)
