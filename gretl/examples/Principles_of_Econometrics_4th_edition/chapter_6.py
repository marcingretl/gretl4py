import math
import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
DATA_PATH = gretl.gretldir + '/examples/Principles_of_Econometrics_4th_edition/data/'
andy    = gretl.get_data(DATA_PATH + 'andy.gdt')
beer    = gretl.get_data(DATA_PATH + 'beer.gdt')
edu_inc = gretl.get_data(DATA_PATH + 'edu_inc.gdt')
cars    = gretl.get_data(DATA_PATH + 'cars.gdt')

############################################
# 6.1. Testing Joint Hypotheses
############################################
print("\n6.1. Testing Joint Hypotheses")
print("\n6.1.1. Testing the Effect of Advertising: the F-Test")
andy.new_series(name='advertsq', value='advert^2')
andy_u = gretl.ols('sales const price advert advertsq').fit()
andy_u.test(type='omit', varlist='advert advertsq', silent=True)
print(andy_u.tests[-1]['summary'])
print(f'keys available for a user: {andy_u.tests[-1].keys()}')

print("\n6.1.2. Testing the Significance of the Model")
F_val = andy_u.Fstat
crit_val = gretl.critical('F', p=0.05, params=[andy_u.ncoeff-1, andy_u.df])
p_val = gretl.pvalue('F', x=F_val, params=[andy_u.ncoeff-1, andy_u.df])
print(f'Fc = {crit_val:.4}')
print(f'F = {F_val:.5}, p-value = {p_val:.5}')

print("\n6.1.4. More General F-Tests")
andy.new_series(name='sales_advert', value='sales - advert')
andy.new_series(name='advertsq_advert', value='advertsq - 3.8*advert')
andy_r = gretl.ols('sales_advert const price advertsq_advert').fit()
F_val = (andy_r.ess - andy_u.ess) / (andy_u.ess / andy_u.df)
p_val = gretl.pvalue('F', x=F_val, params=[1, andy_u.df])
print(f'F = {F_val:.5}, p-value = {p_val:.4}')

############################################
# 6.2. The Use of Nonsample Information
############################################
print("\n6.2. The Use of Nonsample Information")
beer.set_as_default()
BQ = gretl.ols('ln(q) const ln(pb) ln(pl) ln(pr) ln(i)').fit()
BQ.restrict(impose=['b[2] + b[3] + b[4] + b[5] = 0'])           # (6.19)

############################################
# 6.3. Model Speficication
############################################
print("\n6.3. Model Speficication")
print("\n6.3.1. Omitted Variables")
edu_inc.set_as_default()
FAM_hewe = gretl.ols(formula='faminc const he we', simple=True).fit()       # (9.20)
print(FAM_hewe)
FAM_he = gretl.ols(formula='faminc const he', simple=True).fit()            # (9.21)
print(FAM_he)
edu_inc.corr()
FAM_kl = gretl.ols(formula='faminc const he we kl6', simple=True).fit()     # (9.24)
print(FAM_kl)

print("\n6.3.2. Irrelevant Variables")
FAM_irr = gretl.ols(formula='faminc const he we kl6 xtra*', simple=True).fit()
print(FAM_irr)

print("\n6.3.4. Model Selection Criteria")
print("\nTable 6.2")
mdls = [FAM_he, FAM_hewe, FAM_kl, FAM_irr]
for m in [FAM_he, FAM_hewe, FAM_kl, FAM_irr]:
    aic = m.aic/m.T - (1 + math.log(2*math.pi))
    sc = m.bic/m.T - (1 + math.log(2*math.pi))
    print(f"{m.parnames[1:]}, {m.rsq:.4}, {m.adj_rsq:.4}, {aic:.5}, {sc:.5}")
print("\n")
gretl.modeltab(mdls)

print("\n6.3.5. RESET")
FAM_kl.test(type='reset', squares_only=True, quiet=True)
FAM_kl.test(type='reset', quiet=True)

############################################
# 6.4. Poor Data, Collinearity, and Insignificance
############################################
print("\n6.4. Poor Data, Collinearity, and Insignificance")
cars.set_as_default()
CARS_1 = gretl.ols('mpg const cyl', simple=True).fit()
CARS_2 = gretl.ols('mpg const cyl eng wgt', simple=True).fit()
print(CARS_1)
print(CARS_2)
cars.corr()
CARS_2.test(type='vif')
