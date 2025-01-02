import gretl
import pandas

dframe = pandas.read_csv('data9-7.csv')
data97_df = gretl.df2dict(dframe, 4, True)

fname = gretl.gretl_data_path('ramanathan/data9-7.gdt')
data97_gdt = gretl.get_gretl_data(fname)

data97_csv = gretl.csv2dict('data9-7.csv', "4")

#global dataset: will be used later
print("Loading global dataset:")
gretl.attach_data(data97_df)

# we print dataset, i.e. varnames, obs_labes and data
# on previously loaded global dataset
# for the first 4 quarters
print("\nExample using global dataset:")
gretl.sample(t1=1, t2=4)
print('All about data9-7.csv (1975Q1:1975Q4)')
gretl.printdata()
gretl.sample(full=True)

# we print only varnames in bjg.csv
print('Variables in data97_df')
gretl.printdata(namesonly=True)

print("\nExample passing dataset as dict:")
print('Variables in data97_csv')
gretl.printdata(data=data97_csv, namesonly=True)

print('Variables in data97_gdt')
gretl.printdata(data=data97_gdt, namesonly=True)
