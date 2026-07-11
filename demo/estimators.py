import gretl
import numpy as np
import math

np.set_printoptions(precision=4)

#####################################
# helper for floating-point asserts  #
#####################################
def assert_fp_close(actual, expected, name,
                    rel_tol=1e-12, abs_tol=1e-14):
    """
    Robust assertion for floating-point quantities.

    Exact equality of doubles is NOT portable across:
      - operating systems (Windows vs Linux/macOS)
      - compilers
      - BLAS/LAPACK implementations
      - CPU instruction sets

    This helper enforces numerical equality up to a very tight tolerance
    while still catching real regressions.
    """
    assert math.isclose(actual, expected,
                        rel_tol=rel_tol, abs_tol=abs_tol), \
        f"{name} should equal {expected}, is {actual}"

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

#####################################
# load datasets                     #
#####################################
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

gretl.show_data_status()
print("\n")
longley.set_as_default()

#####################################
# ar                                #
#####################################
res = gretl.ar('1 2 ; employ const prdefl gnp armfrc').fit()
assert res.maxlag == ar_maxlag
print(res)

#####################################
# ar1                               #
#####################################
res = gretl.ar1('employ const prdefl gnp').fit()
assert round(res.rho_gls, 10) == ar1_rho_gls
print(res)

#####################################
# sarima                            #
#####################################
bjg.set_as_default()
res = gretl.arima(y='lg', order=(0,1,2), seasonal=(0,1,1), nc=True).fit()
assert np.isnan(res.uhat[0])
print(res)

#####################################
# biprobit                          #
#####################################
greene25_1.set_as_default()
res = gretl.biprobit(
    'anydrg cardhldr const age avgexp ; const age income ownrent selfempl'
    ).fit()
res2 = gretl.biprobit([3, 1, 0, 4, 7, ';', 0, 4, 5, 8, 9]).fit()
assert_fp_close(res.bic, biprobit_bic, "biprobit BIC")
print(res)
print(res2)
print(res2.get_formula())
exit()
#####################################
# dpanel                            #
#####################################
res = gretl.dpanel(
    p=2,
    formula='n  w  k ; GMM(k,2,3) k GMM(w,2,3)',
    data=abdata
).fit()
assert_fp_close(res.ess, dpanel_basic_ess, "dpanel ESS")
print(res)

res = gretl.dpanel(
    p=[2,3],
    formula='n  w  k ; GMM(k,2,3) k GMM(w,2,3)',
    data=abdata,
    two_step=True
).fit()
assert_fp_close(res.ess, dpanel_selected_lags_ess,
                "dpanel selected-lags ESS")
print(res)

#####################################
# duration                          #
#####################################
kennan.set_as_default()
res = gretl.duration('Ti 0 Prod').fit()
assert res.iters == duration_iters
print(res)

res = gretl.duration('Ti 0 Prod', exponential=True).fit()
assert res.iters == duration_ex_iters
print(res)

#####################################
# garch                             #
#####################################
b_g.set_as_default()
res = gretl.garch('1 1 ; Y').fit()
assert_fp_close(res.bic, garch_bic, "garch BIC")
print(res)

#####################################
# intreg                            #
#####################################
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
assert_fp_close(res.bic, intreg_bic, "intreg BIC")
print(res)

#####################################
# heckit                            #
#####################################
mroz87.set_as_default()
res = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE').fit()
assert_fp_close(res.bic, heckit_bic, "heckit BIC")
print(res)

#####################################
# hsk                               #
#####################################
res = gretl.hsk('price ~ 0 + sqft + bedrms', data=data4_1).fit()
assert_fp_close(res.bic, hsk_bic, "hsk BIC")
print(res)

#####################################
# lad                               #
#####################################
gretl.set('seed', 1234)
res = gretl.lad('employ ~ const + prdefl + gnp', data=longley).fit()
assert_fp_close(res.bic, lad_bic, "lad BIC")
print(res)

#####################################
# logistic                          #
#####################################
gretl.set('seed', 1234)
res = gretl.logistic('employ ~ const + prdefl + gnp', data=longley).fit()
assert_fp_close(res.bic, logistic_bic, "logistic BIC")
print(res)

#####################################
# tsls                              #
#####################################
res = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', data=penngrow).fit()
assert_fp_close(res.ess, tsls_ess, "tsls ESS")
print(res)

#####################################
# VAR                               #
#####################################
res = gretl.var(
    order=2, ylist='LRM LRY', xlist='IBO IDE',
    data=denmark, trend=True, seasonals=True
).fit()
assert_fp_close(res.ldet, var_ldet, "VAR log-det")
print(res)

#####################################
# VECM                              #
#####################################
res = gretl.vecm(
    order=2, rank=1, ylist='LRM LRY', xlist='IBO IDE',
    data=denmark, rc=True, seasonals=True
).fit()
assert_fp_close(res.lnl, vecm_lnl, "VECM log-likelihood")
print(res)

#####################################
# wls                               #
#####################################
res = gretl.wls('year employ const prdefl gnp', data=longley).fit()
assert_fp_close(res.bic, wls_bic, "WLS BIC")
print(res)
