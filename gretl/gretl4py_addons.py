import os
import subprocess
import csv
import numpy

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

def csv2dict(FILE, pd=1):
    """turns a cvs into a dictionary gretl understands"""
    d = {}
    data_list = []
    obs_labels = []
    with open(FILE, newline='') as csvfile:
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
    d["data_array"] = data_array
    d["varname"] = varname
    d["obs_labels"] = obs_labels
    d["periodicity"] = pd
    return d

def df2dict(dframe, pd=1, withobs=False):
    '''turns a pandas dataframe into a dictionary gretl understands'''
    d = {}
    # hard rename of 'const' to 'constant'
    dframe.rename(columns={"const": "constant"}, inplace=True)
    mydata = dframe.to_numpy(copy=True)
    if withobs is True:
        d["obs_labels"] = [row[0] for row in mydata]
    else:
        d['obs_labels'] = dframe.axes[0].tolist()
    d['varname'] = dframe.axes[1].tolist()[0+withobs:]
    d['data_array'] = numpy.array([row[0+withobs:] for row in mydata], numpy.double)
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
        varname.insert(const_pos, "constant")
    # we create artificial obs labels
    for i in range(numpy.shape(X)[0]):
        obs_labels.append(i+1)
    d['data_array'] = numpy.hstack((Y, X))
    d['varname'] = varname
    d['obs_labels'] = obs_labels
    d['periodicity'] = pd
    return d

def series2dict (series, pd=1, varname="y"):
    '''turns a pandas series into a dictionary gretl understands'''
    d = {}
    try:
        obs_dates = series.index.to_period()
        d['obs_labels'] = [obs.strftime(_pdtostrformat(pd)) for obs in obs_dates]
    except (AttributeError, TypeError) as err:
        if err:
            print(f'Warning: {err}, fall back to original labels.')
        d['obs_labels'] = series.index.to_list()
    mydata = series.to_numpy(copy=True, dtype=numpy.double)
    d['varname'] = [varname]
    d['data_array'] = mydata
    d['periodicity'] = pd
    return d

def gretl_data_path(fname):
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

def pretty_print(d):
    if isinstance(d, dict):
        if '_gretlprn' in d.keys():
            print(d['_gretlprn'])
        else:
            print('_gretlprn was not found')
    else:
        print('object is not a dictionary')

def gretl_model_print(MODEL):
    '''Function prints libgretl model'''
    DASHEDLINE = "---------------------------------------------------------------"
    if isinstance(MODEL, dict) is not True:
        print("I can print only dictionary!!!\n")
        return -1
    else:
        ratio_type = 'z-score' if MODEL["_zscore"] is True else 't-ratio'
        print(f'\t\tcoefficient\t std. error\t{ratio_type:s}\t p-value\n')
        print('  ' + DASHEDLINE)
        coeff_names = MODEL["coeff"].keys()
        for variable in coeff_names:
            coeff = MODEL["coeff"][variable]
            sderr = MODEL["sderr"][variable]
            ratio = coeff/sderr
            pvalue = MODEL["pvalue"][variable]
            print(f'  {variable:s} \t {coeff:9.4f}\t {sderr:9.4f}\t {ratio:5.3f}\t {pvalue:4.2e}')
        print('')
        print(f'Mean dependent var  {MODEL["ybar"]:9.4f}\t S.D. dependent var  {MODEL["sdy"]:10.4f}')
        print(f'Log-likelihood      {MODEL["lnL"]:9.4f}\t Akaike criterion     {MODEL["criterion"]["aic"]:9.4f}')
        print(f'Schwarz criterion   {MODEL["criterion"]["bic"]:9.4f}\t Hannan-Quinn\t      {MODEL["criterion"]["hqc"]:9.4f}')
        print('\nvcv matrix')
        print(MODEL["vcv"])
        print('')
