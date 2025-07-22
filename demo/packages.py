import importlib.resources as resources
import gretl

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

# load datasets
d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
d2 = gretl.get_data("/home/marcin/.gretl/functions/BMA/examples/FLS_41t72.gdt")

# we load packages
CRIT = gretl.include("criteria")
BMA = gretl.include("BMA")
StrucTiSM = gretl.include("StrucTiSM")

# let's try out criteria
print('##################################################')
print(f'\t\tcriteria')
print('##################################################')
CRIT.show_functions()
# OLS
m1 = gretl.ols(formula="1 0 2 3 4 5 6 7", simple=True).fit()
print(m1)
gretl.genr(name='ess', value=m1.ess)
gretl.genr(name='k', value=m1.ncoeff)
gretl.genr(name='n', value=m1.T)
res1 = gretl.criteria(n=m1.T, k="k", ess0="ess") # arbitrary parameters order!!!!
print(res1)
#gretl.uservars_list()


# let's try out BMA
print('##################################################')
print(f'\t\tBMA')
print('##################################################')
BMA.show_functions()
d2.set_as_default()
d2.new_list(name='Xes', value='2..13')
gretl.genr('bundle OPT = _(Nrep=10^5, model_prior="binomial", Nrank=5)')
#res2 = gretl.BMA(output="bmares", data=d2, Y="GDP_growth", X_list="Xes", quiet=True)
#gretl.BMA_Print(MODEL="&bmares")
#print(type(res2))
p2 = gretl.package(package=BMA, Y="GDP_growth", X_list="Xes", Options="OPT").fit()
print(p2)


# let's try out StrucTiSM
print('##################################################')
print(f'\t\tStrucTiSM')
print('##################################################')
StrucTiSM.show_functions()
setup = gretl.STSM_setup(output="model", data=d1, y="gnp", epsilon=True, trend=1, slope=1)
err = gretl.STSM_estimate(mod="&model")
comps = gretl.STSM_components(mod="model")
print(setup)
print(err)
print(comps)
