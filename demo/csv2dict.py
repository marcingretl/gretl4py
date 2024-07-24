import csv
import numpy

def csv2dict(FILE):
    data_list = list()
    obs_labels = list()
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
    d = dict()
    d["data_array"] = data_array
    d["varname"] = varname
    d["obs_labels"] = obs_labels
    d["periodicity"] = 4
    return d
