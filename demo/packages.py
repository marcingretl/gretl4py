import importlib.resources as resources
import gretl

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

# load datasets
d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
d2 = gretl.get_data("FLS_41t72.gdt", frompkg="BMA")

# we load packages
CRIT = gretl.include("criteria")
StrucTiSM = gretl.include("StrucTiSM")
BMA = gretl.include("BMA")

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
res1 = gretl.criteria(n=m1.T, k="k", ess="ess") # arbitrary parameters order!!!!
print(res1)
#gretl.uservars_list()


# let's try out BMA
print('##################################################')
print(f'\t\tBMA')
print('##################################################')
d2.set_as_default()
#gretl.genr('bundle OPT = _(Nrep=10^5, model_prior="binomial", Nrank=5)')
OPT = gretl.Bundle()
OPT['Nrep'] = 10^5
OPT['model_prior'] = "binomial"
OPT['Nrank'] = 5
d2.new_list(name='Xes', value='2..13')
p2 = gretl.package(name="BMA", Y="GDP_growth", X_list="Xes", Options=OPT).fit()
print(p2)
#p2 = gretl.BMA(Y="GDP_growth", X_list="Xes", data=d2, quiet=True)
#gretl.BMA_Print(MODEL=p2)

# let's try out StrucTiSM
print('##################################################')
print(f'\t\tStrucTiSM')
print('##################################################')
StrucTiSM.show_functions()
setup = gretl.STSM_setup(data=d1, y="gnp", epsilon=True, trend=1, slope=1)
print(setup)
err = gretl.STSM_estimate(mod=setup)
if err:
    print(f'STSM_estimate() error: {err}')
else:
    comps = gretl.STSM_components(mod=setup)
    print(f'components: {comps}')
