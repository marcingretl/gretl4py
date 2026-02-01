import gretl
import numpy as np

def run_example():
  gretl.set('verbose', 'off')

  try:
    bt = gretl.include('BayTool')
  except FileNotFoundError:
    gretl.pkg('install', 'BayTool')
    bt = gretl.include('BayTool')
  bt.show_functions()

  gretl.set('seed', 1234567)

  CASE = 'lm' # lasso, probit, tobit

  if CASE == 'lm': # linear model
    # Koop's textbook ("Bayesian Econemtrics") example on house prices

    d = gretl.get_data('AnglinGencay.gdt', frompkg='BayTool', quiet=True)
    y = 'price'                                                                 # dep. variable
    d.new_list(name='X', value='const lot_size n_bedroom n_bathroom n_storeys') # covariates

    prior = gretl.Bundle()  # prior set-up
    prior['type'] = 'indep' # independent Normal - IG
    prior['beta_mean'] = np.array([0, 10, 5000, 10000, 10000])
    prior['beta_vcv'] = np.diag([10000**2, 25, 2500**2, 5000**2, 5000**2])
    prior['precision_mean'] = 4e-08
    prior['precision_df'] = 5

    opt = gretl.Bundle() # options
    opt['forecast'] = np.array([1, 5000, 2, 2, 1]) # matrix of regressors for prediction
    #opt['thin'] = 1
    #opt['mpi'] = 4
    gretl.set('stopwatch')
    out = gretl.BT_lm(y, 'X', prior, opt)                                     # equivalent to out = gretl.BT_lm(y=y, X='X', prior=prior, opt=opt)
    print(f"Elapsed time in sec: {gretl.get_accessor('$stopwatch'):.2f} sec.")

  elif CASE == 'lasso': # Bayesian LASSO
    # Park and Casella (2008), "The Bayesian LASSO" -JASA-, diabetes example

    d = gretl.get_data('diabetes.gdt', frompkg='BayTool', quiet=True)
    dep = 'y'                                                                 # dep. variable
    d.new_list(name='regr', value='age sex bmi map tc ldl hdl tch ltg glu ')  # covariates - already on the same scale!
    # no need for const term

    prior = gretl.Bundle()  # prior set-up
    prior['type'] = 'hyper' # hyper-prior
    prior['lambda_shape'] = 1
    prior['lambda_rate'] = 1.78

    gretl.set('stopwatch')
    out = gretl.BT_lasso(dep, 'regr', prior, gretl.Bundle(stdize=0))          # equivalent to out = gretl.BT_lasso(y=dep, X='regr', prior=prior, opt=gretl.Bundle(stdize=0))
    print(f"Elapsed time in sec: {gretl.get_accessor('$stopwatch'):.2f} sec.")

  elif CASE == 'probit': # Probit
    # Koop, Poirier, Tobias' textbook ("Bayesian Econometric Methods") example (from Fair, 1978)

    d = gretl.get_data('Fair_Koop.gdt', frompkg='BayTool', quiet=True)
    d.new_list(name='X', value='const male ys_married kids religious ed happy')  # covariates

    prior = gretl.Bundle() # prior for beta
    prior['beta_mean'] = np.zeros((7, 1))
    prior['beta_vcv']  = 100 * np.eye(7)

    opt = gretl.Bundle()
    opt['iter'] = 2000
    opt['burn'] = 500

    gretl.set('stopwatch')
    out = gretl.BT_probit('affair', 'X', prior, opt)                          # equivalent to out = gretl.BT_probit(y='affair', X='X', prior=prior, opt=opt)
    print(f"Elapsed time in sec: {gretl.get_accessor('$stopwatch'):.2f} sec.")

  elif CASE == 'tobit': # Tobit
    # Koop, Poirier, Tobis "Bayesian Econometric Methods" - Replica of Ch. 14, Ex.14.9
    # # Tobit model with double censoring on women worked weeks in a year

    d = gretl.get_data('nlsy_Koop_tobit.gdt', frompkg='BayTool', quiet=True)
    d.new_list(name='X', value='const afqt hi kids ed')

    prior = gretl.Bundle() # prior set-up
    prior['beta_mean'] = np.zeros((5, 1))
    prior['beta_vcv']  = 100 * np.eye(5)
    prior['sigma_a'] = 3
    prior['sigma_b'] = 1/40

    opt = gretl.Bundle()
    opt['iter'] = 10000
    opt['rlimit'] = 52

    gretl.set('stopwatch')
    out = gretl.BT_tobit('weeks', 'X', prior, opt)                            # equivalent to out = gretl.BT_tobit(y='weeks', X='X', prior=prior, opt=opt)
    print(f"Elapsed time in sec: {gretl.get_accessor('$stopwatch'):.2f} sec.")

  else:
    print("This example is not provided yet!\n")

if __name__ == "__main__":
    run_example()
