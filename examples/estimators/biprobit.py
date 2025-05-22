import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('greene25_1.gdt')))
m = gretl.biprobit('anydrg cardhldr const age avgexp ; const age income ownrent selfempl').fit()
print(m)

# print covariance matrix
m = gretl.biprobit(
    'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
    vcv=True).fit()
print(m)

# robust standard errors
m = gretl.biprobit(
    'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
    robust=True).fit()
print(m)

# force use of the OPG method
m = gretl.biprobit(
    'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
    opg=True).fit()
print(m)

# save values of the index functions for the respective equations
m = gretl.biprobit(
    'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
    save_xbeta=True).fit()
print(m)
print(m.bp_yhat)

# print extra information
m = gretl.biprobit(
    'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
    verbose=True).fit()
print(m)
