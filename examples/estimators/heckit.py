import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/mroz87.gdt'))
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE').fit()
print(m)

# suppress printing of results)
# perform two-step estimation
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE', quiet=True).fit()
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
