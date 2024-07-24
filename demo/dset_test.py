import sys
sys.path.append("../")
import gretl
import csv
import numpy
import pandas
from gretl_print import gretl_model_print
from csv2dict import csv2dict
from df2dict import df2dict

DSET = csv2dict('data9-7.csv')
DSET["periodicity"] = 4

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
