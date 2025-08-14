import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
food = gretl.get_data('data/food.gdt')


############################################
# 8.3: Heteroscedasticity-Consistent Standard Errors 
############################################
print('\n\n8.3: Heteroscedasticity-Consistent Standard Errors')
m_food_exp = gretl.ols(formula='food_exp const income', robust=True).fit()
print(m_food_exp)

############################################
# 8.4.1.b: Weighted Least Squares
############################################
food.new_series(name='weight', value='1/income')
wls_food_exp = gretl.wls(formula='weight food_exp const income').fit()
print(wls_food_exp)


