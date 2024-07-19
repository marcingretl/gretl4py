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

#this dataset will be used later
gretl.load_data(data97)

# we print dataset, i.e. varnames, obs_labes and data on previously loaded data
print('All about data9-7.csv')
gretl.printdata()

# we print varnames only and bundled data
print('Variables in bjg-7.csv')
gretl.printdata(data=bjg, varames_only=True)
