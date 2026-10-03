import gretl

def run_example():
  # switch of verbosity
  gretl.set('verbose', 'off')

  # force libgretl to use the decimal point character
  gretl.set('force_decpoint', 'on')

  # datasets
  DATA_PATH = gretl.gretldir + '/examples/Principles_of_Econometrics_4th_edition/data/'
  gdp   = gretl.get_data(DATA_PATH + 'gdp.gdt')
  fred  = gretl.get_data(DATA_PATH + 'fred.gdt')

  ############################################
  # 13.2. Estimating a Vector Error Correction Model
  ############################################
  print("\n13.2. Estimating a Vector Error Correction Model")
  gdp.summary()
  gdp_coint = gdp.coint(order=0, depvar='aus', indepvars='usa', nc=True, verbose=True)      # (13.7-13.8)
  print(f'tau = {gdp_coint["test"]:.3f}')

  EC_au = gretl.ols('aus usa', simple=True).fit()
  EC_au.save_uhat(name='EC')
  print(EC_au)

  VEC_a = gretl.ols('diff(aus) const EC(-1)', simple=True).fit()                            # (13.9)
  VEC_u = gretl.ols('diff(usa) const EC(-1)', simple=True).fit()
  print(VEC_a)
  print(VEC_u)

  ############################################
  # 13.3. Estimating a VAR Model
  ############################################
  print("\n13.3. Estimating a VAR Model")
  fred.set_as_default()
  fred_coint = fred.coint(depvar='c', indepvars='y', order=1, verbose=True)                 # (13.10)
  print(f'tau = {fred_coint["test"]:.3f}')

  var1 = gretl.var(order=1, ylist='diff(c) diff(y)').fit()
  print(var1)

if __name__ == "__main__":
    run_example()
