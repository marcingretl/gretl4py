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

# let's try out criteria
gretl.genr(name='ess', value=m1.ess)
gretl.genr(name='k', value=m1.ncoeff)
gretl.genr(name='n', value=m1.T)
p1 = gretl.package(name="/usr/local/share/gretl/functions/criteria.gfn", ess="ess", n=m1.T, k="k").fit()
print(p1)
print(p1.results)

# let's try out BMA
d1.new_list(name='Xes', value='unemp armfrc pop')
gretl.uservar_print("Xes")
#p2 = gretl.package(name="/home/marcin/.gretl/functions/TestPKG.gfn", Xes="Xes").fit()
p2 = gretl.package(name="/home/marcin/.gretl/functions/BMA/BMA.gfn", fname="BMA", Y="prdefl", X_list="Xes").fit()
print(p2.results)


