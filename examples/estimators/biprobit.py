import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('greene/greene25_1.gdt'))
m = gretl.biprobit('anydrg cardhldr const age avgexp ; const age income ownrent selfempl').fit()
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
