import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('penngrow.gdt')))
m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)').fit()
print(m)

# don’t do diagnostic tests
m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', no_tests=True).fit()
print(m)

# print covariance matrix
m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', vcv=True).fit()
print(m)

# no degrees-of-freedom correction
m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', no_df_corr=True).fit()
print(m)

# robust standard errors
m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', robust=True).fit()
print(m)

# use Limited Information Maximum Likelihood
#m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', liml=True).fit()
#print(m)

# use the Generalized Method of Moments
#m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', gmm=True).fit()
#print(m)
