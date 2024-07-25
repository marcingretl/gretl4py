import sys
sys.path.append("../")
import gretl
import numpy
from gretl4py_addons import gretl_model_print
from gretl4py_addons import csv2dict

''' we load bjg.csv to dict() '''
bjg = csv2dict('bjg.csv', 12)

''' we load data9-7.csv to dict() '''
data97 = csv2dict('data9-7.csv', 4)

print('Loading global dataset: bjg.csv') #this dataset will be used later
gretl.load_data(bjg)

'''Example 1: data is send on function call ('data9-7.csv')'''
print('\nExample 1:')
print('arma(1,1,1,False)\n')
res = gretl.arma(y="QNC", order=(1,1,1), data=data97, print=True)
print('\n\npython level:')
gretl_model_print(res)

print('\nOLS:')
res = gretl.ols('QNC ~ const + PRICE + INCOME', data=data97, print=True)
gretl_model_print(res)

print('arma(1,0,1,print=False)\n')
res = gretl.arma(y="lg", order=(1,1))
gretl_model_print(res)

print('\nOLS:\n')
gretl.load_data(data97)
res = gretl.ols('QNC ~ const + PRICE + INCOME', print=False)
gretl_model_print(res)
