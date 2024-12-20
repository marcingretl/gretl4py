import gretl

bjg = gretl.csv2dict('bjg.csv', 12)
data97 = gretl.csv2dict('data9-7.csv', "4")

#global dataset: will be used later
gretl.attach_data(data97)

# we print dataset, i.e. varnames, obs_labes and data
# on previously loaded global dataset
print('All about data9-7.csv')
gretl.printdata()

# we print only varnames in bjg.csv
print('Variables in bjg.csv')
gretl.printdata(data=bjg, namesonly=True)
