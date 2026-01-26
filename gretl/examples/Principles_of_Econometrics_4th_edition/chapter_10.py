import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
DATA_PATH = gretl.gretldir + '/examples/Principles_of_Econometrics_4th_edition/data/'
mroz = gretl.get_data(DATA_PATH + 'mroz.gdt')

############################################
# 10.2 Cases in Which x and e Are Correlated
############################################
print("\n10.2. Cases in Which x and e Are Correlated")
print("\n10.2.3. Ommited Variables")
mroz.square("exper")
wage_s = gretl.ols("ln(wage) const educ exper sq_exper", simple=True).fit()  # (10.6)

print("\n10.2.4. Least Squares Estimation of a Wage Equation")
print(wage_s)

print("\n10.3.6. Instrumental Variables Estimation of a Wage Equation")
mroz.new_list(name="Z", value="const exper sq_exper")
educ_2sls = gretl.tsls("ln(wage) educ Z ; Z mothereduc").fit()  # (10.26)
print(educ_2sls)
educ_2sls_father = gretl.tsls("ln(wage) educ Z; Z mothereduc fathereduc").fit() # (10.27)
print(educ_2sls_father)
