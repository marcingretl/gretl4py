import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
d1.sample(t1=5, t2=10)
d1.print()

# we restore the full sample
d1.sample(full=True)

# let's use only obs for which armfrc >= 2000
d1.sample(restriction='armfrc>=2000', replace=True)
