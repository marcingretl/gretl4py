import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('b-g.gdt')))
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
