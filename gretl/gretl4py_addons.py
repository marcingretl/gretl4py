import csv
import numpy
import subprocess
import os

def csv2dict(FILE, pd=1):
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
    data_array = numpy.array(data_list, numpy.double)
    d = dict()
    d["data_array"] = data_array
    d["varname"] = varname
    d["obs_labels"] = obs_labels
    d["periodicity"] = pd
    return d

def df2dict(dframe, pd=1, withobs=False):
    '''turns a pandas dataframe into a dictionary gretl understands'''

    d = dict()

    # hard rename of 'const' to 'constant'
    dframe.rename(columns={"const": "constant"}, inplace=True)
    
    mydata = dframe.to_numpy(copy=True)    
    #d["varname"] = dframe.columns[1:]
    #d["data_array"] = numpy.array([row[1:] for row in mydata], numpy.double)
    if withobs is True:
        d["obs_labels"] = [row[0] for row in mydata]
    else:
        d['obs_labels'] = dframe.axes[0].tolist()
    d['varname'] = dframe.axes[1].tolist()[0+withobs:]
    d['data_array'] = numpy.array([row[0+withobs:] for row in mydata], numpy.double)
    d['periodicity'] = pd
    return d

def numpyarray2dict (X, periodicity=1, Y=None, with_const=False,
                     const_pos=1):    
    '''turns a 'raw' numpy array into a dictionary gretl understands
    X - matrix with observations on explanatory variables
    Y - vector with observations on endogeneus variable
    with_const - boolean indicating if X contains constant term
    const_pos - column number which holds constant term values
    '''
    
    if not isinstance(X, numpy.ndarray):
        print("X is not a numpay array!!!")
    if Y is not None:
        if not isinstance(Y, numpy.ndarray):
            print("Y is not a numpay array!!!")
        if numpy.shape(Y)[0] != numpy.shape(X)[0]:
            print("X and Y are not comformable!!!")

    d = dict()
    obs_labels = list()
    varname = list()

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
    d['periodicity'] = periodicity    
    return d

def gretl_data_path(fname):
    s = os.getenv('GRETL_DATA_PATH')
    if s is None:
        res = subprocess.run(['pkg-config', '--cflags-only-I', 'gretl'], stdout=subprocess.PIPE)
        b = res.stdout.split()[0]
        path = b.decode("utf-8")[2:]
        return path[0:path.find("include")] + "share/gretl/data/" + fname
    else:
        return s + '/' + fname

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
        ratio_type = 'z-score' if MODEL["_zscore"] == True else 't-ratio'
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
