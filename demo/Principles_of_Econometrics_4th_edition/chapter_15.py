try:
  from tabulate import tabulate
except ModuleNotFoundError:
  exit('*** \'tabulate\' package required ***\n')
import numpy as np
import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
nls_panel = gretl.get_data('data/nls_panel.gdt')

############################################
# 15.1. A Microeconomic Panel
############################################
print("\n15.1. A Microeconomic Panel")
print("Table 15.1 Representative Observations from NSL Panel Data")
nls_panel.print(series='year lwage educ south black union exper tenure', byobs=True, range="1:15")

############################################
# 15.2. Pooled Model
############################################
print("\n15.2. Pooled Model")
print("\n15.2.2. Pooled Least Squares Estimates of Wage Equation")
spec = 'lwage const educ exper exper2 tenure tenure2 black south union'
POLS = gretl.panel(formula=spec, pooled=True).fit()
POLS_robust = gretl.panel(formula=spec, pooled=True, robust=True).fit()
print("\nTable 15.2 Pooled Least Squares Estimates of Wage Equation")
mdls = [POLS, POLS_robust]
gretl.modeltab(mdls)

############################################
# 15.3. The Fixed Effects Model
############################################
print("\n15.3. The Fixed Effects Model")
print("\n15.3.1. The Least Squares Dummy Variable Estimator for Small N")
nls_panel.sample(t1=1, t2=10, unit=True)
nls_panel.dummify("id")
LSQD_10 = gretl.ols(formula="lwage exper exper2 tenure tenure2 union Did_*", simple=True).fit()            # (15.9)
print(LSQD_10)
print("Table 15.4 Pooled Least Squares Estimates pf Wage Equation for N=10")
LSQD_10_nd = gretl.ols(formula="lwage const exper exper2 tenure tenure2 union", simple=True).fit()
print(LSQD_10_nd)
F = ((LSQD_10.ess - LSQD_10_nd.ess) / (LSQD_10_nd.ncoeff - LSQD_10.ncoeff)) / (LSQD_10.ess / (LSQD_10.T - LSQD_10.ncoeff))   # (15.10)
pval = gretl.pvalue(dist='F', value=F, params=[9,35])
print(f'F = {F:.3f}, pvalue = {pval:.4f}')
nls_panel.sample(full=True)

print("\n15.3.2. The Fixed Effects Estimator")
ALL = ['lwage', 'exper', 'exper2', 'tenure', 'tenure2', 'south', 'union']
for v in ALL:
  nls_panel.new_series(formula=f'{v}_hat = pmean({v})')
  nls_panel.new_series(formula=f'{v}_bar = {v} - {v}_hat')
print("\nTable 15.5 Data in Deviation from Individual Mean Form")
nls_panel.print(series='lwage lwage_hat lwage_bar exper exper_hat exper_bar',
                byobs=True, range="1:15")

print("\n15.3.2a. Fixed Effects Estimates of Wage Equation for N=10")
nls_panel.sample(t1=1, t2=10, unit=True)
LSQ_10_dev = gretl.ols(formula="lwage_bar exper_bar exper2_bar tenure_bar tenure2_bar union_bar",
                       simple=True).fit()
FE_10 = gretl.panel(formula="lwage const exper exper2 tenure tenure2 union").fit()

LSQ_10_dev_coeffs = LSQ_10_dev.coeff
LSQ_10_dev_stderrs = LSQ_10_dev.stderr
LSQ_10_dev_coeffs_aligned = np.insert(LSQ_10_dev_coeffs, 0, None)
LSQ_10_dev_stderrs_aligned = np.insert(LSQ_10_dev_stderrs, 0, None)

data = list(zip(FE_10.parnames, LSQ_10_dev_coeffs_aligned,
                LSQ_10_dev_stderrs_aligned, FE_10.coeff, FE_10.stderr))
print(tabulate(data, headers=["Variable", "Coefficient", "Std. Error", "Coefficient", "Std. Error"],
               tablefmt="plain", floatfmt=".4f"))
nls_panel.sample(full=True)

print("\n15.3.3. Fixed Effects Estimates of Wage Equation from Complete Panel")
FE_nd = gretl.panel(formula="lwage const exper exper2 tenure tenure2 south union").fit()
FE_nd_r = gretl.panel(formula="lwage const exper exper2 tenure tenure2 south union", robust=True).fit()
gretl.modeltab([FE_nd, FE_nd_r])
