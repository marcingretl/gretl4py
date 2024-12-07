"""
 *
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 """

import os
import sys
import subprocess
import csv
import numpy

def _correctvarnames (names):
    """replace const with const_"""
    # in principle we can have a dict with restricted names and their substitutions
    for i in range(len(names)):
        if names[i] == 'const':
            names[i] = 'const_'

def _pdtostrformat (pd):
    """produces date string formatter for given periodicity"""
    res = None
    if pd == 1:
        res = '%Y'
    elif pd == 4:
        res = '%Y:%q'
    elif pd == 12:
        res = '%Y:%m'
    elif pd in range(5,8):
        res = '%Y-%m-%d'
    else:
        raise TypeError(f'date formatter for pd={pd:d} not supported')
    return res

def csv2dict (fname, pd=1):
    """turns a cvs into a dictionary gretl understands"""
    d = {}
    data_list = []
    obs_labels = []
    with open(fname, newline='') as csvfile:
        gretlreader = csv.reader(csvfile, delimiter=',')
        row_nr = 0
        varname = []
        for row in gretlreader:
            row_nr += 1
            if row_nr == 1:
                varname = row[1:]
            else:
                obs_labels.append(row[0])
                data_list.append(row[1:])
    data_array = numpy.array(data_list, numpy.double)
    _correctvarnames(varname)
    d['data_array'] = data_array
    d['varname'] = varname
    d['obs_labels'] = obs_labels
    d['periodicity'] = pd
    return d

def df2dict (dframe, pd=1, withobs=False):
    '''turns a pandas dataframe into a dictionary gretl understands'''
    if 'pandas' not in sys.modules:
        import pandas
    d = {}
    obs_dates = []
    varname = []
    varname = dframe.axes[1].tolist()[0+withobs:]
    mydata = dframe.to_numpy(copy=True)
    if withobs is True:
        obs_dates = [row[0] for row in mydata]
    else:
        obs_dates = dframe.axes[0].tolist()
    #d['obs_labels'] = [obs.strftime(_pdtostrformat(pd)) for obs in obs_dates]
    _correctvarnames(varname)
    d['data_array'] = numpy.array([row[0+withobs:] for row in mydata], numpy.double)
    d['varname'] = varname
    d['obs_labels'] = obs_dates
    d['periodicity'] = pd
    return d

def numpyarray2dict (X, pd=1, Y=None, with_const=False,
                     const_pos=1):
    '''turns a 'raw' numpy array into a dictionary gretl understands
    X - matrix with observations on explanatory variables
    Y - vector with observations on endogeneus variable
    with_const - boolean indicating if X contains constant term
    const_pos - column number which holds constant term values
    '''
    if not isinstance(X, numpy.ndarray):
        raise Exception("X is not a numpay array!!!")
    if Y is not None:
        if not isinstance(Y, numpy.ndarray):
            raise Exception("Y is not a numpay array!!!")
        if numpy.shape(Y)[0] != numpy.shape(X)[0]:
            raise Exception("X and Y are not comformable!!!")
    d = {}
    obs_labels = []
    varname = []
    if Y is not None:
        # force Tx1 vector
        Y = numpy.vstack(Y)
        varname.append("y")
    # we create artificial variables names
    for i in range(with_const, numpy.shape(X)[1]):
        varname.append("x" + str(i+1-with_const))
    if with_const:
        varname.insert(const_pos, "const_")
    # we create artificial obs labels
    for i in range(numpy.shape(X)[0]):
        obs_labels.append(i+1)
    _correctvarnames(varname)
    if Y is not None:
        d['data_array'] = numpy.hstack((Y, X))
    else:
        d['data_array'] = X
    d['varname'] = varname
    d['obs_labels'] = obs_labels
    d['periodicity'] = pd
    return d

def series2dict (series, pd=1, varname="y"):
    '''turns a pandas series into a dictionary gretl understands'''
    if 'pandas' not in sys.modules:
        import pandas
    d = {}
    if varname == 'const':
        varname = 'const_'
    mydata = series.to_numpy(copy=True, dtype=numpy.double)
    d['data_array'] = mydata
    d['varname'] = [varname]
    try:
        obs_dates = series.index.to_period()
        d['obs_labels'] = [obs.strftime(_pdtostrformat(pd)) for obs in obs_dates]
    except (AttributeError, TypeError) as err:
        if err:
            print(f'Warning: {err}, fall back to original labels.')
        d['obs_labels'] = series.index.to_list()
    d['periodicity'] = pd
    return d

def gretl_data_path (fname):
    """return full path to gretl data"""
    res_path = None
    s = os.getenv('GRETL_DATA_PATH')
    if s is None:
        res = subprocess.run(['pkg-config', '--cflags-only-I', 'gretl'], stdout=subprocess.PIPE)
        b = res.stdout.split()[0]
        path = b.decode("utf-8")[2:]
        res_path = path[0:path.find("include")] + "share/gretl/data/" + fname
    else:
        res_path = s + '/' + fname
    return res_path
