import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/kennan.gdt'))
m1 = gretl.duration('Ti ~ const + Prod').fit()
m2 = gretl.duration('Ti const Prod').fit()
m3 = gretl.duration('1 0 2').fit()
print(m1)
print(m2)
print(m3)

# use exponential distribution
m = gretl.duration('Ti ~ const + Prod', exponential=True).fit()
print(m)

# use log-logistic distribution
m = gretl.duration('Ti ~ const + Prod', loglogistic=True).fit()
print(m)

# use log-normal distribution
m = gretl.duration('Ti ~ const + Prod', lognormal=True).fit()
print(m)

# fitted values are medians
m = gretl.duration('Ti ~ const + Prod', medians=True).fit()
print(m)

# robust standard errors
m = gretl.duration('Ti ~ const + Prod', robust=True).fit()
print(m)

# print covariance matrix
m = gretl.duration('Ti ~ const + Prod', vcv=True).fit()
print(m)

# print details of iterations
m = gretl.duration('Ti ~ const + Prod', verbose=True).fit()
print(m)

# don't print anything
m = gretl.duration('Ti ~ const + Prod', quiet=True).fit()
print(m)
