import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/rac3d.gdt'))
m1 = gretl.poisson('DVISITS ~ const + SEX + AGE').fit()
m2 = gretl.poisson('DVISITS const SEX AGE').fit()
m3 = gretl.poisson('13 0 1 2').fit()
print(m1)
print(m2)
print(m3)

# robust standard errors
m = gretl.poisson('DVISITS ~ const + SEX + AGE', robust=True).fit()
print(m)

# print covariance matrix
m = gretl.poisson('DVISITS ~ const + SEX + AGE', vcv=True).fit()
print(m)

# print details of iterations
m = gretl.poisson('DVISITS ~ const + SEX + AGE', verbose=True).fit()
print(m)

# don't print results
m = gretl.poisson('DVISITS ~ const + SEX + AGE', quiet=True).fit()
print(m)
