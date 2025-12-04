import gretl
d = gretl.get_data("klein.gdt")

d.new_series("W = Wp + Wg")
d.new_series("A = t + (1918 - 1931)")
d.new_series("K1 = K(-1)")

IDNS = [
  'P = X - T - Wp',
  'W = Wp + Wg',
  'X = C + I + G',
  'K = K1 + I'
  ]
EQNS = ['C 0 P P(-1) W', 'I 0 P P(-1) K1', 'Wp 0 X X(-1) A']

# OLS
system_ols = gretl.system(method="ols", equations=EQNS, endogs="C I Wp P W X K", identities=IDNS).fit()
print(system_ols)
# add uhat to dataset
uhat_name = system_ols.save_uhat(eqn=1)
print(f'residuals added as: {uhat_name}')

# TSLS
system_tsls = gretl.system(method="tsls", equations=EQNS, endogs="C I Wp P W X K", identities=IDNS).fit()
print(system_tsls)


# 3SLS
system_3sls = gretl.system(method="3sls", equations=EQNS, endogs="C I Wp P W X K", identities=IDNS).fit()
print(system_3sls)

# fiml
system_fiml = gretl.system(method="fiml", equations=EQNS, endogs="C I Wp P W X K", identities=IDNS, verbose=True).fit()
print(system_fiml)

# liml
system_liml = gretl.system(method="liml", equations=EQNS, endogs="C I Wp P W X K", identities=IDNS).fit()
print(system_liml)

# sur
system_sur = gretl.system(method="sur", equations=EQNS, endogs="C I Wp P W X K", identities=IDNS).fit()
print(system_sur)

# RESTRICT
print("\n****************************")
print("restrict")
system_sur.restrict(impose=['b[1,1] = 0', 'b[1,2] - b[2,2] = 0', 'b[3,4] = -5'], verbose=True)
print(system_sur.restrictions[-1]['pvalue'])
