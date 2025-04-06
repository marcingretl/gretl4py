import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/penngrow.gdt'))
m1 = gretl.panel('Y ~ const + Y(-1) + X').fit()
m2 = gretl.panel('Y const Y(-1) X').fit()
print(m1)
print(m2)

# print covariance matrix
m = gretl.panel('Y ~ const + Y(-1) + X', vcv=True).fit()
print(m)

# estimate with group fixed effects
m = gretl.panel('Y ~ const + Y(-1) + X', fixed_effects=True).fit()
print(m)

# random effects or GLS model
m = gretl.panel('Y ~ const + Y(-1) + X', random_effects=True).fit()
print(m)

# use the Nerlove transformation
#m = gretl.panel('Y ~ const + Y(-1) + X', nerlove=True).fit()
#print(m)

# estimate via pooled OLS
m = gretl.panel('Y ~ const + Y(-1) + X', pooled=True).fit()
print(m)

# estimate the between-groups model
m = gretl.panel('Y ~ const + Y(-1) + X', between=True).fit()
print(m)

# robust standard errors
m = gretl.panel('Y ~ const + Y(-1) + X', robust=True).fit()
print(m)

# include time dummy variables
m = gretl.panel('Y ~ const + Y(-1) + X', time_dummies=True).fit()
print(m)

# weighted least squares
m = gretl.panel('Y ~ const + Y(-1) + X', unit_weights=True).fit()
print(m)

# iterative estimation
m = gretl.panel('Y ~ const + Y(-1) + X', iterate=True).fit()
print(m)

# compute Hausman test via matrix difference
m = gretl.panel('Y ~ const + Y(-1) + X', matrix_diff=True).fit()
print(m)

# more verbose output
m = gretl.panel('Y ~ const + Y(-1) + X', verbose=True).fit()
print(m)
