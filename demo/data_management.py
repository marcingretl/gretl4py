import importlib.resources as resources
import gretl
import pandas

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

dframe = pandas.read_csv('data9-7.csv')
data97_dict = gretl.df2dict(dframe, 4, True)
data97_df = gretl.get_data(data97_dict)
data97_gdt = gretl.get_data(str(data_dir.joinpath('data9-7.gdt')))
data97_csv = gretl.get_data('data9-7.csv')

#global dataset: will be used later
print("Setting 'data97_df' as default dataset:")
data97_df.set_as_default()

# we print dataset, i.e. varnames, obs_labes and data
# on previously loaded global dataset
# for the first 4 quarters
print("\nExample using default dataset:")
data97_df.sample(t1=1, t2=4)

print('All about data9-7.csv (1975Q1:1975Q4)')
data97_df.print()
data97_df.sample(full=True)

# we print only varnames in bjg.csv
print('Variables in data97_df')
print(data97_df.varnames())

print('Variables in data97_gdt')
print(data97_gdt.varnames())

print('Variables in data97_csv')
print(data97_csv.varnames())
