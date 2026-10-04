import gretl
import numpy as np

def run_example():
  # check gretl4py version
  assert float(gretl.version) >= 0.41, "required gretl4py 0.41 or higher"

  # switch of verbosity
  gretl.set('verbose', 'off')

  # force libgretl to use the decimal point character
  gretl.set('force_decpoint', 'on')

  # datasets
  DATA_PATH = gretl.gretldir + '/examples/Principles_of_Econometrics_4th_edition/data/'
  food = gretl.get_data(DATA_PATH + 'food.gdt')
  cps2 = gretl.get_data(DATA_PATH + 'cps2.gdt')
  coke = gretl.get_data(DATA_PATH + 'coke.gdt')

  ############################################
  # 8.1. The Nature of Heteroscedasticity
  ############################################
  print("\n8.1. The Nature of Heteroscedasticity")
  FOOD_f = gretl.ols(formula='food_exp const income', simple=True).fit()            # (8.3)
  print(FOOD_f)

  ############################################
  # 8.2. Detecting Heteroscedasticity
  ############################################
  print("\n8.2. Detecting Heteroscedasticity")
  print("\n8.2.2b. Testing the Food Expenditure Example")
  FOOD_f.save_uhat_sq(name='FOOD_f_e_sq')
  FOOD_fe = gretl.ols(formula='FOOD_f_e_sq const income', simple=True).fit()        # (8.17)
  crit_val = gretl.critical(c='x', p=0.05, params=FOOD_fe.ncoeff-1)
  print(f'chi^2 = {FOOD_fe.trsq:.3}, critical value = {crit_val:.3}')
  FOOD_f.white()

  print("\n8.2.3. The Goldfeld-Quandt Test")
  cps2.set_as_default()
  spec = 'wage const educ exper metro'
  CPS_f = gretl.ols(formula=spec, simple=True).fit()          # (8.19)
  print(CPS_f)
  cps2.sample('metro == 1', restrict=True)
  CPS_m = gretl.ols(formula=spec, simple=True).fit()          # (8.20a)
  cps2.sample('metro == 0', restrict=True, replace=True)
  CPS_r = gretl.ols(formula=spec, simple=True).fit()          # (8.20b)
  cps2.sample(full=True)
  Flc = gretl.critical(c='F', p=0.975, params=[CPS_m.df, CPS_r.df])
  Fuc = gretl.critical(c='F', p=0.025, params=[CPS_m.df, CPS_r.df])
  F_val = CPS_m.sigma**2 / CPS_r.sigma**2
  print(f'Flc = {Flc:.2}, Fuc = {Fuc:.3}')
  print(f'F = {F_val:.3}')

  print("\n8.2.3a. The Food Expenditure Example")
  food.set_as_default()
  food.sample(1, 20)
  FOOD_1 = gretl.ols(formula=FOOD_f.get_formula()).fit()
  food.sample(21, 40)
  FOOD_2 = gretl.ols(formula=FOOD_f.get_formula()).fit()
  food.sample(full=True)
  F_val = FOOD_2.sigma**2 / FOOD_1.sigma**2
  crit_val = gretl.critical(c='F', p=0.05, params=[FOOD_2.df, FOOD_1.df])
  print(f'F = {F_val:.3}, critical value = {crit_val:.3}')

  ############################################
  # 8.3. Heteroscedasticity-Consistent Standard Errors
  ############################################
  print("\n8.3. Heteroscedasticity-Consistent Standard Errors")
  FOOD_we = gretl.ols(formula=FOOD_f.get_formula(), robust=True).fit()
  gretl.modeltab([FOOD_we, FOOD_f])

  ############################################
  # 8.4. Generalized Least Squares: Known Form of Variance
  ############################################
  print("\n8.4. Generalized Least Squares: Known Form of Variance")
  print("\n8.4.1.b. Weighted Least Squares")
  food.new_series(name='w', value='1/income')

  print("\n8.4.1.c. Food Expenditure Estimates")
  FOOD_we = gretl.wls(formula='w food_exp const income').fit()    # (8.32)
  print(FOOD_we)

  print("\n8.4.2. Grouped Data")
  cps2.set_as_default()
  cps2.new_series(name='rural', value=f'1 - metro')
  cps2.new_series(name='wm', value=f'{CPS_m.sigma} * metro')
  cps2.new_series(name='wr', value=f'{CPS_r.sigma} * rural')
  cps2.new_series(name='w', value='1 / (wm + wr)^2')
  CPS_gr = gretl.wls(formula='w ' + CPS_f.get_formula()).fit()    # (8.35)
  print(CPS_gr)

  ############################################
  # 8.5. Generalized Least Squares: Unknown Form of Variance
  ############################################
  print("\n8.5. Generalized Least Squares: Unknown Form of Variance")
  food.set_as_default()
  food.new_list(name='x', value='const income')
  food.new_list(name='z', value='const log(income)')
  food.logs('FOOD_f_e_sq')
  FOOD_l = gretl.ols('l_FOOD_f_e_sq z').fit()
  alpha = FOOD_l.coeff.numpy_view
  const = food.get_series('const')
  l_income = food.get_series('l_income')
  z = np.column_stack((const, l_income))
  lincomb = z @ alpha
  food.add_series(data=lincomb, name='lincomb')
  food.new_series(name='w', value='1 / exp(lincomb)')
  FOOD_wls = gretl.wls(formula='w ' + FOOD_f.get_formula()).fit()    # (8.47)
  print(FOOD_wls)

  ############################################
  # 8.6. Heteroscedasticity in the Linear Probability Model
  ############################################
  print("\n8.6. Heteroscedasticity in the Linear Probability Model")
  print("\n8.6.1. The Marketing Example Revisited")
  coke.set_as_default()
  print(gretl.summary(simple=True))
  coke.new_list(name='x', value='const pratio disp_coke disp_pepsi')
  # LS
  LS = gretl.ols('coke x').fit()
  # LS-robust
  gretl.set('hc_version', 0)
  LS_robust = gretl.ols('coke x', robust=True).fit()
  # GLS-omit
  yhat_name = LS.save_yhat()
  coke.new_series(f'p = {yhat_name}')
  coke.new_series('varp = p*(1-p)')
  coke.new_series('pos = (varp>0)')
  coke.new_series('w = pos * 1/varp')
  coke.sample('pos', dummy=True)
  GLS_omit = gretl.wls('w coke x').fit()
  coke.sample(full=True)
  # GLS-trunc
  coke.new_series('b = (p<.01) || (p>.99)')
  coke.new_series('pt = b*0.01 + p*(1-b)')
  coke.new_series('varp_t = pt*(1-pt)')
  coke.new_series('w_t = 1/varp_t')
  GLS_trunc = gretl.wls('w_t coke x').fit()
  print()
  gretl.modeltab([LS, LS_robust, GLS_trunc, GLS_omit])

if __name__ == "__main__":
    run_example()
