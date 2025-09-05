import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
utown = gretl.get_data('data/utown.gdt')
cps4_small = gretl.get_data('data/cps4_small.gdt')
coke = gretl.get_data('data/coke.gdt')
star = gretl.get_data('data/star.gdt')
njmin3 = gretl.get_data('data/njmin3.gdt')


############################################
# 7.1: Indicator Variables
############################################
print("\n7.1: Indicator Variables")
print("\n7.1.3: An Example: The University Effect on House Prices")
utown.new_series(name='sqftutown', value='sqft * utown')
UNIV = gretl.ols(formula='price const utown sqft sqftutown age pool fplace').fit()
print(UNIV)

############################################
# 7.2: Applying Indicator Variables
############################################
print("\n7.2: Applying Indicator Variables")
print("\n7.2.1: Interactions Between Qualitative Factors")
cps4_small.set_as_default()
cps4_small.new_series(name='blackfemale', value='black * female')
WAGE_u = gretl.ols('wage const educ black female blackfemale', simple=True).fit()   # (7.8)
print(WAGE_u)
WAGE_r = gretl.ols('wage const educ', simple=True).fit()
j_val = WAGE_u.ncoeff - WAGE_r.ncoeff
F_val = ((WAGE_r.ess - WAGE_u.ess)/j_val) / (WAGE_u.ess / WAGE_u.df)
crit_val = gretl.critical('F', value=0.01, params=[j_val, WAGE_u.df])
print(f'F = {F_val:.4}, critical value = {crit_val:.3}')

print("\n7.2.2: Qualitative Factors with Several Categories")
spec = 'wage const educ black female blackfemale south midwest west'
WAGE_reg = gretl.ols(spec, simple=True).fit()   # (7.8)
print(WAGE_reg)
WAGE_reg.test(type='omit', vars='south midwest west')
print(f'critival value: {gretl.critical('F', value=0.01, params=[3, WAGE_reg.df]):.5}')

print("\n7.2.3: Testing the Equivalence of Two Regressions")
cps4_small.new_series(name='educsouth', value='educ * south')
cps4_small.new_series(name='blacksouth', value='black * south')
cps4_small.new_series(name='femalesouth', value='female * south')
cps4_small.new_series(name='blackfemalesouth', value='blackfemale * south')
spec = 'wage const educ black female blackfemale south educsouth blacksouth femalesouth blackfemalesouth'
WAGE_full = gretl.ols(spec, simple=True).fit()                         # (7.10)
print(WAGE_full)
cps4_small.sample(restriction='south == 0')
WAGE_nonsouth = gretl.ols('wage const educ black female blackfemale', simple=True).fit()
cps4_small.sample(full=True)
cps4_small.sample(restriction='south == 1')
WAGE_south = gretl.ols('wage const educ black female blackfemale', simple=True).fit()
cps4_small.sample(full=True)
print()
mdls = [WAGE_full, WAGE_nonsouth, WAGE_south]
gretl.modeltab(mdls)
WAGE_full.test(type='omit', vars='south educsouth blacksouth femalesouth blackfemalesouth')

############################################
# 7.3: Log-Linear Models
############################################
print("\n7.3: Log-Linear Models")
WAGE_loglin = gretl.ols('log(wage) const educ female', simple=True).fit()       # (7.11)
print(WAGE_loglin)

############################################
# 7.4: The Linear Probablitity Model
############################################
print("\n7.4: The Linear Probablitity Model")
coke.set_as_default()
CO = gretl.ols('coke const pratio disp_coke disp_pepsi', simple=True).fit()
print(CO)

############################################
# 7.5: Treatment Effects
############################################
print("\n7.5: Treatment Effects")
star.set_as_default()
star.new_list(name='v', value='totalscore small tchexper boy freelunch white_asian tchwhite tchmasters schurban schrural')
gretl.summary(varlist='v', by='small', simple=True)
gretl.summary(varlist='v', by='regular', simple=True)

print("\n7.5.3: Application of Difference Estimation: Project STAR")
star.sample(restriction='aide == 0')
ST_1 = gretl.ols('totalscore const small', simple=True).fit()   # (7.15)

print("\n7.5.4: The Difference Estimator with Additional Controls")
ST_2 = gretl.ols('totalscore const small tchexper', simple=True).fit()  # (7.16)

print("\n7.5.4a: School Fixed Effects")
ST_3 = gretl.ols('totalscore const small dummify(schid)', simple=True).fit()
ST_4 = gretl.ols('totalscore const small tchexper dummify(schid)', simple=True).fit() # (7.17)
print()
mdls = [ST_1, ST_2, ST_3, ST_4]
gretl.modeltab(mdls)

print("\n7.5.4b: Linear Probability Model Check of Random Assignment")
ST_s = gretl.ols('small const boy white_asian tchexper freelunch', simple=True).fit()
print(ST_s)

print("\n7.5.6: Estimating the Effect of a Wage Change")
njmin3.set_as_default()
njmin3.new_list(name='v1', value='nj d d_nj')
njmin3.new_list(name='v2', value='kfc roys wendys co_owned')
njmin3.new_list(name='v3', value='southj centralj pa1')
NJ_1 = gretl.ols('fte const v1', simple=True).fit()        # (7.22)
NJ_2 = gretl.ols('fte const v1 v2', simple=True).fit()
NJ_3 = gretl.ols('fte const v1 v2 v3', simple=True).fit()
print()
mdls = [NJ_1, NJ_2, NJ_3]
gretl.modeltab(mdls)

print("\n7.5.7: Using Panel Data")
NJ_demp = gretl.ols(formula='demp const nj', simple=True).fit()
print(NJ_demp)
