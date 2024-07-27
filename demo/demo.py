import sys
sys.path.append("../")
import gretl
import numpy
from gretl4py_addons import csv2dict
from gretl4py_classes import *


''' we load bjg.csv to dict() '''
bjg = csv2dict('bjg.csv', 12)

''' we load data9-7.csv to dict() '''
data97 = csv2dict('data9-7.csv', 4)

print('Loading global dataset: bjg.csv') # this dataset will be used later
gretl.load_data(bjg)

'''Example 1: data is send on function call ('data9-7.csv')'''
print('\nExample 1:')
print('arma(1,1,1,False)\n')
res = Model(gretl.arma(y="QNC", order=(1,1,1), data=data97, print=True))
print('\n\npython level:')
print(res)

print('\nOLS:')
res = Model(gretl.ols('QNC ~ const + PRICE + INCOME', data=data97, print=True))
print(res)

'''Example 2: we use data send before function call ('bjg.csv')'''
print('\nExample 2:')
print('arma(1,0,1,print=False)\n')
res = Model(gretl.arma(y="lg", order=(1,1)))
print(res)

print('\nOLS:\n')
gretl.load_data(data97)
res = Model(gretl.ols('QNC ~ const + PRICE + INCOME', print=False))
print(res)
