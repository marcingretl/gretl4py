import gretl

#assertions
ols_bic = 146.9358272370446
ols_lnl = -69.50932762409941


#####################################
# ols                               #
#####################################
fname = gretl.gretl_data_path('ramanathan/data4-1.gdt')
mydata = gretl.get_gretl_data(fname)

print('OLS without options:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata).fit()
assert res.bic() == ols_bic, 'BIC should equal ' + str(ols_bic)
print(res)

print('OLS with robust option:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata, robust=True).fit()
assert res.lnl() == ols_lnl, 'log-likelihood should equal ' + str(ols_bic)
print(res)
############################################################################


#####################################
# logit and probit                  #
#####################################
fname = gretl.gretl_data_path('greene/greene19_1.gdt')
ldata = gretl.get_gretl_data(fname)

print('LOGIT without options:')
res = gretl.logit('GRADE ~ 0 GPA TUCE PSI', data=ldata).fit()
print(res)

print('PROBIT example with p-value option:')
res = gretl.probit('GRADE ~ 0 GPA TUCE PSI', data=ldata, p_values=True).fit()
print(res)
print('normality test:')
print(res.results['normality_test'])
############################################################################


#####################################
# duration                          #
#####################################
fname = gretl.gretl_data_path('misc/kennan.gdt')
gretl.attach_gretl_data(fname)

print('DURATION without options:')
res = gretl.duration('Ti 0 Prod').fit()
print(res)

print('DURATION: exponential:')
res = gretl.duration('Ti 0 Prod', exponential=True).fit()
print(res)
############################################################################


#####################################
# sarima                            #
#####################################
gretl.attach_gretl_data('bjg.csv')

print('SARIMA (0 1 2, 0 1 1):')
res = gretl.arima(y='lg', order=(0,1,2), seasonal=(0,1,1), nc=True).fit()
print(res)
############################################################################


#####################################
# multiple precision ols            #
#####################################
print('OLS with multiple precision floating-point')

fname = gretl.gretl_data_path('misc/longley.gdt')
gretl.attach_gretl_data(fname)

res = gretl.mpols('1 0 2 3 4 5 6 7').fit()
print(res)
############################################################################


#####################################
# panel                             #
#####################################
fname = gretl.gretl_data_path('misc/abdata.gdt')
gretl.attach_gretl_data(fname)

print('PANEL: fixed effects')
res = gretl.panel('n ~ 0 + w + k + ys').fit()
print(res)

print('PANEL: random effects')
res = gretl.panel('n ~ 0 + w + k + ys', random_effects=True).fit()
print(res)
############################################################################
