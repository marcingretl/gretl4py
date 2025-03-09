import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/b-g.gdt'))
m = gretl.garch('1 1 ; Y').fit()
print(m)

# robust standard errors
m = gretl.garch('1 1 ; Y', robust=True).fit()
print(m)

# print details of iterations
m = gretl.garch('1 1 ; Y', verbose=True).fit()
print(m)

# print covariance matrix
m = gretl.garch('1 1 ; Y', vcv=True).fit()
print(m)

# model without constant
m = gretl.garch('1 1 ; Y', nc=True).fit()
print(m)

# standardized residuals
m = gretl.garch('1 1 ; Y', stdresid=True).fit()
print(m)

# use Fiorentini, Calzolari, Panattoni algorithm
m = gretl.garch('1 1 ; Y', fcp=True).fit()
print(m)

# initial variance parameters from ARMA
m = gretl.garch('1 1 ; Y', arma_init=True).fit()
print(m)
