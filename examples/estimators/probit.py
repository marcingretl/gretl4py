import gretl

d1 = gretl.get_data(gretl.gretl_data_path('greene/greene22_2.gdt'))
m1 = gretl.probit('Y ~ const + Z1 + Z2').fit()
m2 = gretl.probit('Y const Z1 Z2').fit()
m3 = gretl.probit('1 0 2 3').fit()
print(m1)
print(m2)
print(m3)

# robust standard errors
m = gretl.probit('Y ~ const + Z1 + Z2', robust=True).fit()
print(m)

# print covariance matrix
m = gretl.probit('Y ~ const + Z1 + Z2', vcv=True).fit()
print(m)

# print details of iterations
m = gretl.probit('Y ~ const + Z1 + Z2', verbose=True).fit()
print(m)

# show p-values instead of slopes
m = gretl.probit('Y ~ const + Z1 + Z2', p_values=True).fit()
print(m)

# select pseudo-R-squared variant
m = gretl.probit('Y ~ const + Z1 + Z2', estrella=True).fit()
print(m)

# estimates a random effects panel probit model
#m = gretl.probit('Y ~ const + Z1 + Z2', random_effects=True).fit()
#print(m)
