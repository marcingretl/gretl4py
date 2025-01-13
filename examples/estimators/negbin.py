import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/rac3d.gdt'))
m1 = gretl.negbin('DVISITS ~ const + SEX + AGE').fit()
m2 = gretl.negbin('DVISITS const SEX AGE').fit()
m3 = gretl.negbin('13 0 1 2').fit()
print(m1)
print(m2)
print(m3)

# use NegBin 1 model
m = gretl.negbin('DVISITS ~ const + SEX + AGE', model1=True).fit()
print(m)

# robust standard errors
m = gretl.negbin('DVISITS ~ const + SEX + AGE', robust=True).fit()
print(m)

# force use of the OPG method
m = gretl.negbin('DVISITS ~ const + SEX + AGE', opg=True).fit()
print(m)

# print details of iterations
m = gretl.negbin('DVISITS ~ const + SEX + AGE', verbose=True).fit()
print(m)
