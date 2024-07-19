import sys
sys.path.append("../")
import gretl
import csv
import numpy
from gretl_print import gretl_model_print

''' we load bjg.csv to dict() '''
bjg = dict()
data_list = list()
obs_labels = list()
FILE = 'bjg.csv'
with open(FILE, newline='') as csvfile:
    gretlreader = csv.reader(csvfile, delimiter=',')
    row_nr = 0
    varname = list()
    for row in gretlreader:
        row_nr += 1
        if row_nr == 1:
            varname = row[1:]
        else:
            obs_labels.append(row[0])
            data_list.append(row[1:])

data_array = numpy.asfortranarray(numpy.array(data_list, numpy.double))
#data_array = numpy.array(data_list, numpy.double)
bjg["data_array"] = data_array
bjg["varname"] = varname
bjg["obs_labels"] = obs_labels
bjg["periodicity"] = 12 # may be string

''' we load data9-7.csv to dict() '''
data97 = dict()
data_list = list()
obs_labels = list()
FILE = 'data9-7.csv'
with open(FILE, newline='') as csvfile:
    gretlreader = csv.reader(csvfile, delimiter=',')
    row_nr = 0
    varname = list()
    for row in gretlreader:
        row_nr += 1
        if row_nr == 1:
            varname = row[1:]
        else:
            obs_labels.append(row[0])
            data_list.append(row[1:])

data_array = numpy.asfortranarray(numpy.array(data_list, numpy.double))
data97["data_array"] = data_array
data97["varname"] = varname
data97["obs_labels"] = obs_labels
data97["periodicity"] = "4"

print('Loading global dataset: bjg.csv') #this dataset will be used later
gretl.load_data(bjg)

'''Example 1: data is send on function call ('data9-7.csv')'''
print('\nExample 1:')
print('arma(1,0,1,False)\n')
res = gretl.arma(data=data97, y="QNC", p=1, q=1, d=0, print=True)
print('\n\npython level:')
gretl_model_print(res)

print('\nOLS:')
res = gretl.ols(data=data97, y="QNC", x1="PRICE", x2="INCOME", print=True)
gretl_model_print(res)

'''Example 2: we use data send before function call ('bjg.csv')'''
print('\nExample 2:')
print('arma() [with default options]\n')
res = gretl.arma(y="g")
gretl_model_print(res)

print('arma(1,0,1,False)\n')
res = gretl.arma(y="lg", p=1, q=1, print=False)
gretl_model_print(res)

#print('\nOLS:\n')
#res = gretl.ols(y="QNC", x1="PRICE", x2="INCOME", print=False)
#gretl_model_print(res)
