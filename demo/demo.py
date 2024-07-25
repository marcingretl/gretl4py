import sys
sys.path.append("../")
import gretl
import numpy
from gretl4py_addons import gretl_model_print
from gretl4py_addons import csv2dict

''' we load bjg.csv to dict() '''
bjg = csv2dict('bjg.csv')

''' we load data9-7.csv to dict() '''
data97 = csv2dict('data9-7.csv')

print('Loading global dataset: bjg.csv') #this dataset will be used later
gretl.load_data(bjg)

'''Example 1: data is send on function call ('data9-7.csv')'''
print('\nExample 1:')
print('arma(1,1,1,False)\n')
res = gretl.arma(data=data97, y="QNC", p=1, q=1, d=1, print=True)
print('\n\npython level:')
gretl_model_print(res)

print('\nOLS:')
res = gretl.ols('QNC ~ const + PRICE + INCOME', data=data97, print=True)
gretl_model_print(res)

'''Example 2: we use data send before function call ('bjg.csv')'''
print('\nExample 2:')
print('arma() [with default options] ~ arma(1,0,0,False)\n')
res = gretl.arma(y="g")
gretl_model_print(res)

print('arma(1,0,1,False)\n')
res = gretl.arma(y="lg", p=1, q=1, print=False)
gretl_model_print(res)

print('\nOLS:\n')
gretl.load_data(data97)
res = gretl.ols('QNC ~ const + PRICE + INCOME', print=False)
gretl_model_print(res)
