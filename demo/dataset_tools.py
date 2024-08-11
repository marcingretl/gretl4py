import gretl

bjg = gretl.csv2dict('bjg.csv', 12)
data97 = gretl.csv2dict('data9-7.csv', "4")

#this dataset will be used later
gretl.load_data(data97)

# we print dataset, i.e. varnames, obs_labes and data on previously loaded data
print('All about data9-7.csv')
gretl.printdata()

# we print varnames only and bundled data
print('Variables in bjg.csv')
gretl.printdata(data=bjg, varamesonly=True)
