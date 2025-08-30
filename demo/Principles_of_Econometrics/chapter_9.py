import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
usmacro = gretl.get_data('data/usmacro.gdt')
okun = gretl.get_data('data/okun5_aus.gdt')

############################################
# 9.2.2: Okun's Law
############################################
print('\n9.2.2: Okun\'s Law')
okun.sample(t1='1985:2', t2='1986:4') #2009:3
okun.diff('u')
okun.new_series(formula='u_1 = u(-1)') # to be modified - use of 'lags' command
okun.new_series(formula='g_1 = g(-1)') # to be modified - use of 'lags' command
okun.new_series(formula='g_rate = (g - g_1) / g_1 * 100')

print(okun.print(byobs=True))






#usmacro.test(type="corrgm", y="g", order=45)
#print(usmacro.tests[-1]['pvalue'])