import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
edu_inc = gretl.get_data('data/edu_inc.gdt')

# list of variables
variables = edu_inc.varnames()
print(variables)


############################################
# 6.3.5: RESET
############################################
print('\n\n6.3.5: RESET')
print('model 6.24')
faminc = gretl.ols(formula='faminc const he we kl6').fit()
print(faminc)
faminc.test(type="reset", squares_only=True, quiet=True)
faminc.test(type="reset", quiet=True)


