import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('tobit.gdt')))
m1 = gretl.tobit('apt ~ const + read + math + proggeneral + progvocational', llimit=400, rlimit=600.7).fit()
print(m1)
