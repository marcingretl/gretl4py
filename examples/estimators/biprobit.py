import gretl

d1 = gretl.get_data(gretl.gretl_data_path('greene/greene25_1.gdt'))
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
