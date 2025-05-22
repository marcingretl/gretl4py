import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
d1.sample(t1=5, t2=10)     # select a subsample from time 5 to time 10
d1.print()                 # print the current subsample

# we restore the full sample
d1.sample(full=True)       # reset the sample to full dataset

# let's use only obs for which armfrc >= 2000
d1.sample(restriction='armfrc>=2000', replace=True)  # restrict sample to obs satisfying armfrc >= 2000, replacing previous sample
