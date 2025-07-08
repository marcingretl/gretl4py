import importlib.resources as resources
import gretl

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

# load datasets
d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))

# OLS
m1 = gretl.ols(formula="1 0 2 3 4 5 6 7").fit()
print(m1)

# for testing only
#p = gretl.package(name="/home/marcin/.gretl/functions/TestPKG.gfn", S=1.1).fit()
#print(type(p.results))
#print(p.results)
#exit()

# let's try out criteria
gretl.genr(name='ess', value=m1.ess)
gretl.genr(name='k', value=m1.ncoeff)
gretl.genr(name='n', value=m1.T)
p1 = gretl.package(name="/usr/local/share/gretl/functions/criteria.gfn", ess="ess", n=m1.T, k="k").fit()
print(p1)
#print(type(p1.results))
#exit()

# let's try out BMA
BMA_PATH="/home/marcin/.gretl/functions/BMA/BMA.gfn"
gretl.package_functions(name=BMA_PATH)
#exit()
d2 = gretl.get_data("/home/marcin/.gretl/functions/BMA/examples/FLS_41t72.gdt")
d2.set_as_default()
d2.new_list(name='Xes', value='2..10')
p2 = gretl.package(name=BMA_PATH, fname="BMA", Y="GDP_growth", X_list="Xes").fit()
print(p2)
print(p2.attrs())
#p2.functions()

