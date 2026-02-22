import gretl
import numpy as np

np.set_printoptions(precision=4)

#####################################
# benchmark results                 #
#####################################
ar_maxlag = 2
ar1_rho_gls = 0.2616616334
biprobit_bic = 2506.4099430756182
dpanel_basic_ess = 9.022209618194166
dpanel_selected_lags_ess = 7.911977166149402
duration_iters = 6
duration_ex_iters = 4
garch_bic = 2251.154787743446
heckit_bic = 3210.116451720699
hsk_bic = 51.979783893886214
intreg_bic = 811.2237450583831
lad_bic = 259.34905425955566
logistic_bic = -39.699260872353044
logit_fXb = 0.188902177165472
logit_addsratios = [
    [16.87971482698798, 21.318085199908165, 1.4201941279029757, 200.6238210977191],
    [1.099832242458331, 0.15568587945550874, 0.8333650615466961, 1.4515018895871261],
    [10.790732404989523, 11.487427998195665, 1.3393441542871571, 86.93800280038114]
]
midas_bic = 183.15280526534264
mpols_const = -3.482258634595818 * 10**6
negbin_bic = 7097.157041004623
ols_bic = 146.9358272370446
ols_lnl = -69.50932762409941
panel_fixed_effects_F_test = 123.0227754885
panel_hausman_test = 56.19204895
poisson_bic = 7804.511447436008
probit_normtest_pval = 0.16442606965149
quantreg_bic = 80.09721340480137
tobit_bic = 859.3742496817283
tsls_ess = 8.019127932937186
var_ldet = -16.441467597306143
vecm_lnl = 277.6983198360378
wls_bic = 379.860968614329

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# load datasets
abdata     = gretl.get_data('abdata.gdt')
bjg        = gretl.get_data('bjg.csv')
b_g        = gretl.get_data('b-g')
data4_1    = gretl.get_data('data4-1.gdt')
denmark    = gretl.get_data('denmark.gdt')
greene19_1 = gretl.get_data('greene19_1.gdt')
greene25_1 = gretl.get_data('greene25_1.gdt')
kennan     = gretl.get_data('kennan.gdt')
longley    = gretl.get_data('longley.csv')
midas      = gretl.get_data('gdp_midas.gdt')
mroz87     = gretl.get_data('mroz87.gdt')
mrw        = gretl.get_data('mrw.gdt')
penngrow   = gretl.get_data('penngrow.gdt')
rac3d      = gretl.get_data('rac3d.gdt')
tobit      = gretl.get_data('tobit.gdt')
wtp        = gretl.get_data('wtp.gdt')

# list available datasets
gretl.show_data_status()
print("\n")
longley.set_as_default()

#####################################
# ar                                #
#####################################
res = gretl.ar('1 2 ; employ const prdefl gnp armfrc').fit()
assert res.maxlag == ar_maxlag, f'maxlag should equal {ar_maxlag:d}'
print(res)

#####################################
# ar1                               #
#####################################
res = gretl.ar1('employ const prdefl gnp').fit()
assert round(res.rho_gls, 10) == ar1_rho_gls, f'rho_gls should equal {ar1_rho_gls:f}, is {res.rho_gls:f}'
print(res)

#####################################
# sarima                            #
#####################################
bjg.set_as_default()
print('SARIMA(0 1 2)(0 1 1):')
res = gretl.arima(y='lg', order=(0,1,2), seasonal=(0,1,1), nc=True).fit()
assert np.isnan(res.uhat[0]), 'the first element should be NaN!!!'
print(res)

#####################################
# biprobit                          #
#####################################
greene25_1.set_as_default()
print('BIPROBIT')
res = gretl.biprobit('anydrg cardhldr const age avgexp ; const age income ownrent selfempl').fit()
assert res.bic == biprobit_bic, f'bic should equal {biprobit_bic:f}, is {res.bic:f}'
print(res)

#####################################
# dpanel                            #
#####################################
print('DPANEL')
res = gretl.dpanel(p=2, formula='n  w  k ; GMM(k,2,3) k GMM(w,2,3)', data=abdata).fit()
assert res.ess == dpanel_basic_ess, f'ess should equal {dpanel_basic_ess:f}, is {res.ess:f}'
print(res)

print('DPANEL: selected lags')
res = gretl.dpanel(p=[2,3], formula='n  w  k ; GMM(k,2,3) k GMM(w,2,3)', data=abdata, two_step=True).fit()
assert res.ess == dpanel_selected_lags_ess, f'ess should equal {dpanel_selected_lags_ess:f}, is {res.ess:f}'
print(res)

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

#####################################
# garch                             #
#####################################
b_g.set_as_default()
print('GARCH')
res = gretl.garch('1 1 ; Y').fit()
assert res.bic == garch_bic, f'bic should equal {garch_bic:f}, is {res.bic:f}'
print(res)

#####################################
# intreg                            #
#####################################
print('INTREG:')
wtp.sample('depvar == 1', restrict=True)
wtp.new_series('hi = bidl')
wtp.sample('depvar == 2', restrict=True, replace=True)
wtp.new_series('lo = bidl')
wtp.new_series('hi = bid1')
wtp.sample('depvar == 3', restrict=True, replace=True)
wtp.new_series('lo = bid1')
wtp.new_series('hi = bidh')
wtp.sample('depvar == 4', restrict=True, replace=True)
wtp.new_series(name='lo', value='bidh')
wtp.sample(full=True)
res = gretl.intreg('lo hi const age female income', data=wtp).fit()
assert res.bic == intreg_bic, f'bic should equal {intreg_bic:f}, is {res.bic:f}'
print(res)

#####################################
# heckit                            #
#####################################
mroz87.set_as_default()
print('HECKIT')
res = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE').fit()
assert res.bic == heckit_bic, f'bic should equal {heckit_bic:f}, is {res.bic:f}'
print(res)

#####################################
# hsk                               #
#####################################
print('HSK without options:')
res = gretl.hsk('price ~ 0 + sqft + bedrms', data=data4_1).fit()
assert res.bic == hsk_bic, f'bic should equal {hsk_bic:f}'
print(res)

#####################################
# lad                               #
#####################################
print('LAD')
gretl.set('seed', 1234)
res = gretl.lad('employ ~ const + prdefl + gnp', data=longley).fit()
assert res.bic == lad_bic, f'bic should equal {lad_bic:f}, is {res.bic:f}'
print(res)

#####################################
# logistic                          #
#####################################
print('LOGISTIC')
gretl.set('seed', 1234)
res = gretl.logistic('employ ~ const + prdefl + gnp', data=longley).fit()
assert res.bic == logistic_bic, f'bic should equal {logistic_bic:f}, is {res.bic:f}'
print(res)

#####################################
# logit                             #
#####################################
print('LOGIT without options:')
res = gretl.logit('GRADE ~ 0 GPA TUCE PSI', data=greene19_1).fit()
assert [[round(el, 10) for el in row] for row in res.oddsratios] == [[round(el, 10) for el in row] for row in logit_addsratios],\
 'oddsratios matrix should be:\n' + str(logit_addsratios)
print(res)
print("Odds Ratios:")
res.oddsratio()

#####################################
# midasreg                          #
#####################################
print('MIDAS:')
gretl.genr('theta = {1,5}')
#gretl.new_matrix(name='theta', value='{1,5}')
#midas.new_list('dX = ld_payems*')
midas.new_list(name='dX', value='ld_payems*')
midas.new_list('dXL = hflags(3, 11, dX)')
midas.sample('1985:1', '2009:1')
res = gretl.midasreg(formula='ld_qgdp 0 ld_qgdp(-1) ; mdsl(dXL, 2, theta)', data=midas).fit()
assert res.bic == midas_bic, f'bic should equal {midas_bic:f}, is {res.bic:f}'
print(res)
print('midas_info:')
print(res.midas_info)

#####################################
# mols                              #
#####################################
longley.set_as_default()
print('OLS with multiple precision floating-point')
res = gretl.mpols('1 0 2 3 4 5 6 7').fit()
print(res.coeff)
assert res.coeff[0].round(9) == mpols_const, f'const should equal {mpols_const:f}, is {format(res.coeff[0])}'
print(res)

#####################################
# negbin                            #
#####################################
print('NEGBIN')
res = gretl.negbin('DVISITS ~ const + SEX + AGE', data=rac3d).fit()
assert res.bic == negbin_bic, f'bic should equal {negbin_bic:f}, is {res.bic:f}'
print(res)

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

#####################################
# poisson                           #
#####################################
print('POISSON')
res = gretl.poisson('DVISITS ~ const + SEX + AGE', data=rac3d).fit()
assert res.bic == poisson_bic, f'bic should equal {poisson_bic:f}, is {res.bic:f}'
print(res)

#####################################
# probit                            #
#####################################
print('PROBIT example with p-value option:')
res = gretl.probit('GRADE ~ 0 GPA TUCE PSI', data=greene19_1, p_values=True).fit()
assert (round(res.normality_test['pvalue'], 14) == probit_normtest_pval), 'pvalue in normality of residuals should equal ' + str(probit_normtest_pval)
print(res)
print('normality test:')
print(res.normality_test)

#####################################
# quantreg                          #
#####################################
print('QUANTREG')
mrw.new_series('ly60 = log(gdp60)')
mrw.new_series('dlny = log(gdp85) - ly60')
mrw.new_series('ngd = 0.05 + (popgrow/100)')
mrw.new_series('lngd = log(ngd)')
mrw.new_series('linv = log(i_y/100)')
mrw.new_series('lschool = log(school/100)')
mrw.sample('nonoil', dummy=True)
res = gretl.quantreg(0.75, 'dlny const ly60 linv lngd lschool', data=mrw).fit()
assert res.bic == quantreg_bic, f'bic should equal {quantreg_bic:f}, is {res.bic:f}'
print(res)

#####################################
# tobit                             #
#####################################
print('TOBIT')
res = gretl.tobit('apt ~ const + read + math + proggeneral + progvocational', llimit=400, rlimit=600.7, data=tobit).fit()
assert res.bic == tobit_bic, f'bic should equal {tobit_bic:f}, is {res.bic:f}'
print(res)

#####################################
# tsls                              #
#####################################
print('TSLS')
res = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', data=penngrow).fit()
assert res.ess == tsls_ess, f'ess should equal {tsls_ess:f}, is {res.ess:f}'
print(res)

#####################################
# VAR                               #
#####################################
print('VAR')
res = gretl.var(order=2, ylist='LRM LRY', xlist='IBO IDE', data=denmark, trend=True, seasonals=True).fit()
assert res.ldet == var_ldet, f'ldet should equal {var_ldet:f}, is {res.ldet:f}'
print(res)

#####################################
# VECM                              #
#####################################
print('VECM')
res = gretl.vecm(order=2, rank=1, ylist='LRM LRY', xlist='IBO IDE', data=denmark, rc=True, seasonals=True).fit()
assert res.lnl == vecm_lnl, f'lln should equal {vecm_lnl:f}, is {res.lnl:f}'
print(res)

#####################################
# wls                               #
#####################################
print('WLS')
res = gretl.wls('year employ const prdefl gnp', data=longley).fit()
assert res.bic == wls_bic, f'bic should equal {wls_bic:f}, is {res.bic:f}'
print(res)
