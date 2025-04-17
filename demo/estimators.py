import gretl
import numpy as np
np.set_printoptions(precision=4)

#####################################
# benchmark results                 #
#####################################
ar_maxlag = 2
ar1_rho_gls = 0.2616616334
duration_iters = 6
duration_ex_iters = 4
mpols_const = -3.482258634595818 * 10**6
ols_bic = 146.9358272370446
ols_lnl = -69.50932762409941
panel_fixed_effects_F_test = 123.0227754885
panel_hausman_test = 56.19204895
logit_fXb = 0.188902177165472
probit_normtest_pval = 0.16442606965149
logit_addsratios = [[16.87971482698798, 21.318085199908165, 1.4201941279029757, 200.6238210977191],
 [1.099832242458331, 0.15568587945550874, 0.8333650615466961, 1.4515018895871261],
 [10.790732404989523, 11.487427998195665, 1.3393441542871571, 86.93800280038114]]
#assert (round(res.fXb, 15) == logit_fXb), 'f(beta\'x) at mean of independent vars should equal: ' + str(logit_fXb)
############################################################################


# we force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')


# we load datasets
longley = gretl.get_data(gretl.gretl_data_path('misc/longley.gdt'))
bjg = gretl.get_data('bjg.csv')
fname = gretl.gretl_data_path('misc/kennan.gdt')
kennan = gretl.get_data(fname)
fname = gretl.gretl_data_path('greene/greene19_1.gdt')
greene19_1 = gretl.get_data(fname)
fname = gretl.gretl_data_path('ramanathan/data4-1.gdt')
data4_1 = gretl.get_data(fname)
fname = gretl.gretl_data_path('misc/abdata.gdt')
abdata = gretl.get_data(fname)

# list available datasets
gretl.show_data_status()
print("\n")

############################################################################
longley.set_as_default()

#####################################
# ar                                #
#####################################
res = gretl.ar('1 2 ; employ const prdefl gnp armfrc').fit()
assert res.maxlag == ar_maxlag, f'maxlag shuld equal {ar_maxlag:d}'
print(res)

#####################################
# ar1                               #
#####################################
res = gretl.ar1('employ const prdefl gnp').fit()
assert round(res.rho_gls, 10) == ar1_rho_gls, f'rho_gls shuld equal {ar1_rho_gls:f}, is {res.rho_gls:f}'
print(res)
############################################################################


#####################################
# sarima                            #
#####################################
bjg.set_as_default()

print('SARIMA (0 1 2, 0 1 1):')
res = gretl.arima(y='lg', order=(0,1,2), seasonal=(0,1,1), nc=True).fit()
assert np.isnan(res.uhat[0]), 'the first element should be NaN!!!'
print(res)
############################################################################


#####################################
# duration                          #
#####################################
kennan.set_as_default()

print('DURATION without options:')
res = gretl.duration('Ti 0 Prod').fit()
assert res.iters == duration_iters, f'no of iters should be {duration_iters:d}, is {res.iters:d}'
print(res)

print('DURATION: exponential:')
res = gretl.duration('Ti 0 Prod', exponential=True).fit()
assert res.iters == duration_ex_iters, f'no of iters should be {duration_iters:d}, is {res.iters:d}'
print(res)
############################################################################


#####################################
# logit                             #
#####################################
print('LOGIT without options:')
res = gretl.logit('GRADE ~ 0 GPA TUCE PSI', data=greene19_1).fit()
assert [[round(el, 10) for el in row] for row in res.oddsratios] == [[round(el, 10) for el in row] for row in logit_addsratios],\
 'oddsratios matrix should be:\n' + str(logit_addsratios)
print(res)
print("Odds Ratios:")
print(res.oddsratio())

#####################################
# probit                            #
#####################################
print('PROBIT example with p-value option:')
res = gretl.probit('GRADE ~ 0 GPA TUCE PSI', data=greene19_1, p_values=True).fit()
assert (round(res.normality_test['pvalue'], 14) == probit_normtest_pval), 'pvalue in normality of residuals should equal ' + str(probit_normtest_pval)
print(res)
print('normality test:')
print(res.normality_test)
############################################################################


#####################################
# mols                              #
#####################################
longley.set_as_default()

print('OLS with multiple precision floating-point')
res = gretl.mpols('1 0 2 3 4 5 6 7').fit()
print(res.coeff)
assert res.coeff[0].round(9) == mpols_const, f'const should equal {mpols_const:f}, is {format(res.coeff[0])}'
print(res)
############################################################################


#####################################
# ols                               #
#####################################
print('OLS without options:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=data4_1).fit()
assert res.bic == ols_bic, f'BIC should equal {ols_bic:f}'
print(res)

print('OLS with robust option:')
res = gretl.ols('price ~ 0 + sqft + bedrms', data=data4_1, robust=True).fit()
assert round(res.lnl, 8) == round(ols_lnl, 8), f'log-likelihood should equal {ols_lnl:f}'
print(res)
############################################################################


#####################################
# panel                             #
#####################################
abdata.set_as_default()

print('PANEL: fixed effects')
res = gretl.panel('n ~ 0 + w + k + ys').fit()
assert round(res.fixed_effects_F['test'], 10) == panel_fixed_effects_F_test, \
    f'F test for fixed effects should equal {panel_fixed_effects_F_test:f}, \
    is {res.fixed_effects_F["test"]:f}'
print(res)

print('PANEL: random effects')
res = gretl.panel('n ~ 0 + w + k + ys', random_effects=True).fit()
assert round(res.hausman_test['test'], 8) == panel_hausman_test, \
    f'F test for fixed effects should equal {panel_hausman_test:f}, \
    is {res.hausman_test["test"]:f}'
print(res)
############################################################################
