def df2dict(dframe, pd=1):
    '''turns a pandas dataframe into a dictionary gretl understands'''
    
    mydata = dframe.to_numpy(copy=True)
    test = dict()
    test["varname"] = dframe.columns[1:]
    test["data_array"] = [row[1:] for row in mydata]
    test["periodicity"] = pd
    test["obs_labels"] = [row[0] for row in mydata]

    return test
