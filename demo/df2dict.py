def df2dict(dframe):
    '''turns a pandas dataframe into a dictionary gretl understands'''
    
    mydata = dframe.values
    test = dict()
    test["varname"] = dframe.columns[1:]
    test["data_array"] = [row[1:] for row in mydata]
    test["periodicity"] = 4
    test["obs_labels"] = [row[0] for row in mydata]

    return test
