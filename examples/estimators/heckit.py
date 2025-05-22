import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('mroz87.gdt')))
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE').fit()
print(m)

# perform two-step estimation
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE', two_step=True).fit()
print(m)

# print covariance matrix)
# force use of the OPG method
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE', vcv=True).fit()
print(m)

# force use of the OPG method
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE', opg=True).fit()
print(m)

# robust standard errors
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE', robust=True).fit()
print(m)

# print extra output
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE', verbose=True).fit()
print(m)
