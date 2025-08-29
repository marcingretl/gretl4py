import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
usmacro = gretl.get_data('data/usmacro.gdt')

############################################
# 9.2.2: Okun's Law
############################################
print('\n9.2.2: Okun\'s Law')
usmacro.diff("u")
print(usmacro.print(byobs=True, summary=True))
