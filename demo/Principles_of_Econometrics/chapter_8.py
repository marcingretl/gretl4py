import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
food = gretl.get_data('data/food.gdt')
coke = gretl.get_data('data/coke.gdt')

############################################
# 8.3: Heteroscedasticity-Consistent Standard Errors
############################################
print('\n\n8.3: Heteroscedasticity-Consistent Standard Errors')
print(gretl.summary(simple=True))
m_food_exp = gretl.ols(formula='food_exp const income', robust=True).fit()
print(m_food_exp)

############################################
# 8.4.1.b: Weighted Least Squares
############################################
food.new_series(name='weight', value='1/income')
wls_food_exp = gretl.wls(formula='weight food_exp const income').fit()
print(wls_food_exp)

############################################
# 8.6: Heteroscedasticity in the Linear Probability Model
############################################
print('\n\n8.6: Heteroscedasticity in the Linear Probability Model')
coke.set_as_default()
print(gretl.summary(simple=True))
coke.new_list(name='x', value='const pratio disp_coke disp_pepsi')

# LS
LS = gretl.ols('coke x').fit()

# LS-robust
gretl.set('hc_version', 0)
LS_robust = gretl.ols('coke x', robust=True).fit()

# GLS-omit
yhat_name = LS.add_yhat()
coke.new_series(f'p = {yhat_name}')
coke.new_series('varp = p*(1-p)')
coke.new_series('pos = (varp>0)')
coke.new_series('w = pos * 1/varp')
coke.sample(restriction='pos')
GLS_omit = gretl.wls('w coke x').fit()
coke.sample(full=True)

# GLS-trunc
coke.new_series('b = (p<.01) || (p>.99)')
coke.new_series('pt = b*0.01 + p*(1-b)')
coke.new_series('varp_t = pt*(1-pt)')
coke.new_series('w_t = 1/varp_t')
GLS_trunc = gretl.wls('w_t coke x').fit()

print(LS)
print(LS_robust)
print(GLS_omit)
print(GLS_trunc)
