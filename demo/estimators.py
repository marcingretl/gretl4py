import gretl
import numpy as np
np.set_printoptions(precision=4)

#####################################
# assertions                        #
#####################################
ols_bic = 146.9358272370446
ols_lnl = -69.50932762409941
logit_fXb = 0.188902177165472
probit_normtest_pval = 0.16442606965149
logit_addsratios = np.array([[1.6879714826987978e+01, 2.1318085199908165e+01, 1.4201941279029757e+00, 2.0062382109771909e+02],
                             [1.0998322424583311e+00, 1.5568587945550874e-01, 8.3336506154669610e-01, 1.4515018895871261e+00],
                             [1.0790732404989523e+01, 1.1487427998195665e+01, 1.3393441542871571e+00, 8.6938002800381142e+01]])
#assert (res.results['oddsratios'] == logit_addsratios).all(), 'oddsratios matrix should be:\n' + str(logit_addsratios)
############################################################################


# we force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')


#####################################
# ols                               #
#####################################
fname = gretl.gretl_data_path('ramanathan/data4-1.gdt')
mydata = gretl.get_gretl_data(fname)

print('OLS without options:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata).fit()
assert res.bic == ols_bic, 'BIC should equal ' + str(ols_bic)
print(res)

print('OLS with robust option:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata, robust=True).fit()
assert res.lnl == ols_lnl, 'log-likelihood should equal ' + str(ols_bic)
print(res)
############################################################################


#####################################
# logit and probit                  #
#####################################
fname = gretl.gretl_data_path('greene/greene19_1.gdt')
ldata = gretl.get_gretl_data(fname)

print('LOGIT without options:')
res = gretl.logit('GRADE ~ 0 GPA TUCE PSI', data=ldata).fit()

assert (round(res.fXb, 15) == logit_fXb), 'f(beta\'x) at mean of independent vars should equal: ' + str(logit_fXb)
print(res)
print("Odds Ratios:")
print("columns: odds ratio, std. error, low95, high95")
print("rows:\t GPA, TUCE, PSI")
print(res.oddsratios)

print('PROBIT example with p-value option:')
res = gretl.probit('GRADE ~ 0 GPA TUCE PSI', data=ldata, p_values=True).fit()
assert (round(res.normality_test['pvalue'], 14) == probit_normtest_pval), 'pvalue in normality of residuals should equal ' + str(probit_normtest_pval)
print(res)
print('normality test:')
print(res.normality_test)
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
assert res.uhat[0] == 'NA', 'the first element should be NaN!!!'
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
