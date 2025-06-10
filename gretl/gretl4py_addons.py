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
import inspect
import gretl

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

def df2dict (dframe, pd=1, with_obs=False):
    '''turns a pandas dataframe into a dictionary gretl understands'''
    if 'pandas' not in sys.modules:
        import pandas
    d = {}
    obs_dates = []
    varname = []
    varname = dframe.axes[1].tolist()[0+with_obs:]
    mydata = dframe.to_numpy(copy=True)
    if with_obs is True:
        obs_dates = [row[0] for row in mydata]
    else:
        obs_dates = dframe.axes[0].tolist()
    #d['obs_labels'] = [obs.strftime(_pdtostrformat(pd)) for obs in obs_dates]
    _correctvarnames(varname)
    d['data_array'] = numpy.array([row[0+with_obs:] for row in mydata], numpy.double)
    d['varname'] = varname
    d['obs_labels'] = obs_dates
    d['periodicity'] = pd
    return d

def nparray2dict (X, pd=1, Y=None, with_const=False,
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

def fc_print (fc):
    try:
        from tabulate import tabulate
    except ModuleNotFoundError:
        print('*** warning: fc_print() requires \'tabulate\' package ***\n')
        return
    print(f'\nForecast for obs {fc["t1"]} to {fc["t2"]} with {fc["confidence"]} percent interval\n')
    print(tabulate(fc['data'], headers=fc['data_labels'], tablefmt="plain"))
    print("")
    print(f'Forecast evaluation statistics\n')
    print(tabulate(fc['stats'], headers=[], showindex=fc['stats_labels'], tablefmt="plain"))
    print("")

def print_module_source (mod):
    """
    Prints the full source code of the given imported module.

    Parameters:
        mod: imported module (not a string)
    """
    try:
        source_file = inspect.getsourcefile(mod)
        if source_file is None:
            print("Source file not found for the module.")
            return
        with open(source_file, "r") as f:
            code = f.read()
        print(code)
    except Exception as e:
        print(f"Error while trying to read source: {e}")
