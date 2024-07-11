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
with open('data9-7.csv', newline='') as csvfile:
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
stobs = obs_labels[0].replace("Q", ":", 1)
endobs = obs_labels[-1].replace("Q", ":", 1)
DSET["data_array"] = data_array
DSET["varname"] = varname
DSET["obs_labels"] = obs_labels
DSET["stobs"] = stobs
DSET["endobs"] = endobs

# arma(2,1,0)
res = gretl.dsettest(DSET, 2, 1, 0, 1)
print("\n\npython level:")
gretl_model_print(res)
