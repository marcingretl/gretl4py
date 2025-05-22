import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
m1 = gretl.lad('employ ~ const + prdefl + gnp').fit()
m2 = gretl.lad('employ const prdefl gnp').fit()
m3 = gretl.lad('1 0 2 3').fit()
print(m1)
print(m2)
print(m3)

# print covariance matrix
m = gretl.lad('employ ~ const + prdefl + gnp', vcv=True).fit()
print(m)

# don't compute covariance matrix
m = gretl.lad('employ ~ const + prdefl + gnp', no_vcv=True).fit()
print(m)

