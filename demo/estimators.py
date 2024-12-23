import gretl

fname = gretl.gretl_data_path('ramanathan/data4-1.gdt')
mydata = gretl.get_gretl_data(fname)

print('OLS without options:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata).fit()
print(res)

print('OLS with robust option:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata, robust=True).fit()
print(res)

fname = gretl.gretl_data_path('greene/greene19_1.gdt')
ldata = gretl.get_gretl_data(fname)

print('logit example:')
res = gretl.logit('GRADE ~ 0 GPA TUCE PSI', data=ldata).fit()
print(res)

print('probit example with p-value option:')
res = gretl.probit('GRADE ~ 0 GPA TUCE PSI', data=ldata, p_values=True).fit()
print(res)
print('normality test:')
print(res.results['normality_test'])

# load dataset using libgretl and set as global DSET
fname = gretl.gretl_data_path('misc/kennan.gdt')
gretl.attach_gretl_data(fname)

print('\nduration: Kennan example variants:\n')
res = gretl.duration('Ti 0 Prod').fit()
print(res)
res = gretl.duration('Ti 0 Prod', exponential=True).fit()
print(res)

# replace the global dataset
gretl.attach_gretl_data('bjg.csv')
print('seasonal arma (0 1 2, 0 1 1):')
res = gretl.arima(y='lg', order=(0,1,2), seasonal=(0,1,1), nc=True).fit()
print(res)


# mpols
fname = gretl.gretl_data_path('misc/longley.gdt')
gretl.attach_gretl_data(fname)
res = gretl.mpols('1 0 2 3 4 5 6 7').fit()
print(res)


# panel
fname = gretl.gretl_data_path('misc/abdata.gdt')
gretl.attach_gretl_data(fname)

print('panel, fixed effects')
res = gretl.panel('n ~ 0 + w + k + ys').fit()
print(res)

print('panel, random effects')
res = gretl.panel('n ~ 0 + w + k + ys', random_effects=True).fit()
print(res)
