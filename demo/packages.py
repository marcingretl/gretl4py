import importlib.resources as resources
import gretl

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

# load datasets
d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
d2 = gretl.get_data("/home/marcin/.gretl/functions/BMA/examples/FLS_41t72.gdt")

# load packages
CRIT = gretl.include("criteria")
BMA = gretl.include("BMA")
StrucTiSM = gretl.include("StrucTiSM")
BMA.show_functions()
StrucTiSM.show_functions()


# for testing only
#p = gretl.package(name="/home/marcin/.gretl/functions/TestPKG.gfn", S=1.1).fit()
#print(type(p.results))
#print(p.results)
#exit()

# let's try out criteria
# OLS
m1 = gretl.ols(formula="1 0 2 3 4 5 6 7").fit()
print(m1)
gretl.genr(name='ess', value=m1.ess)
gretl.genr(name='k', value=m1.ncoeff)
gretl.genr(name='n', value=m1.T)
res1 = gretl.criteria(ess="ess", n=m1.T, k="k")
print(res1)
#gretl.uservars_list()

# let's try out BMA
BMA.show_functions()
d2.set_as_default()
d2.new_list(name='Xes', value='2..13')
gretl.genr('bundle OPT = _(Nrep=10^4, model_prior="beta-binomial")')
res2 = gretl.BMA(output="bmares", data=d2, Y="GDP_growth", X_list="Xes", quiet=True)
#print(type(res2))
gretl.BMA_Print(MODEL="&bmares")
#p2 = gretl.package(package=BMA, data=d2, Y="GDP_growth", X_list="Xes", Options="OPT").fit()
#p2 = gretl.package(package=BMA, Y="GDP_growth", X_list="Xes").fit()
#print(p2)


# let's try out StrucTiSM
gretl.STSM_setup(output="model", data=d1, y="gnp")
gretl.STSM_estimate(mod="&model")
gretl.STSM_components(mod="model")
