import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
andy = gretl.get_data('data/andy.gdt')

# list of variables
variables = andy.varnames()
print(variables)


############################################
# 5.2.2: OLS
############################################
m1 = gretl.ols(formula='sales ~ const + price + advert').fit()
print(m1)

# alternative ways of defining a model
m2 = gretl.ols('sales ~ 0 + price + advert').fit()
m3 = gretl.ols('sales const price advert').fit()
m4 = gretl.ols('sales 0 price advert').fit()
m5 = gretl.ols('1 const 2 3').fit()
m6 = gretl.ols('1 0 2 3').fit()
m7 = gretl.ols('1 ~ const + 2 + 3').fit()
m8 = gretl.ols('1 ~ 0 + 2 + 3').fit()

# using gretl list
andy.new_list(name='Xes1', value='price advert')
andy.new_list(name='Xes2', value='2 3')

m9 = gretl.ols('sales const Xes1').fit()
m10 = gretl.ols('1 0 Xes2').fit()

# coefficients estimates
coeff = m1.coeff
print(f'coefficients estimates:\n{coeff}')

# standard errors
stderr = m1.stderr
print(f'\nstdandard errors:\n{stderr}')

# covariance matrix
vcv = m1.vcv
print(f'\ncovariance matrix:\n{vcv}')


############################################
# 5.5.3: linear restrictions
############################################
print('\n\n5.5.3: linear restrictions')
m1.restrict(impose=['-0.2*b[2] -0.5*b[3] = 0'])
print(f'-0.2*b[2] -0.5*b[3] > 0, pvalue: {m1.restrictions[-1]['pvalue']/2 : .3f}')


############################################
# 5.6.2: extending the model
############################################
print('\n\n5.6.2: extending the model')
andy.new_series(name='advertsq', value='advert^2')
msq = gretl.ols(formula='sales ~ const + price + advert + advertsq').fit()
print(msq)
