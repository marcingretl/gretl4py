import csv
import numpy

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
    data_array = numpy.asfortranarray(numpy.array(data_list, numpy.double))
    d = dict()
    d["data_array"] = data_array
    d["varname"] = varname
    d["obs_labels"] = obs_labels
    d["periodicity"] = pd
    return d

def df2dict(dframe, pd=1):
    '''turns a pandas dataframe into a dictionary gretl understands'''

    mydata = dframe.to_numpy(copy=True)
    d = dict()
    d["varname"] = dframe.columns[1:]
    d["data_array"] = numpy.asfortranarray(numpy.array([row[1:] for row in mydata], numpy.double))
    d["periodicity"] = pd
    d["obs_labels"] = [row[0] for row in mydata]

    return d

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

