import sys
sys.path.append("../")
import gretl
import csv
import numpy
from gretl_print import gretl_model_print

''' we load CSV do dict() '''
DSET = dict()
data_list = list()
obs_labels = list()
FILE = 'bjg.csv' #data9-7.csv
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
DSET["data_array"] = data_array
DSET["varname"] = varname
DSET["obs_labels"] = obs_labels
DSET["periodicity"] = 12 # may be string

'''Example 1: data is send on function call'''
res = gretl.arma(data=DSET, p=2, d=1, q=0, print=True)
print("\n\npython level:")
gretl_model_print(res)

'''Example 2: data is send before function call'''
print("\n\n\nLoading global dataset:")
gretl.load_data(DSET)
print("arma() [with default options]\n")
res = gretl.arma()
gretl_model_print(res)

print("arma(1,0,1,False) [with default options]\n")
res = gretl.arma(p=1, q=1, print=False)
gretl_model_print(res)
