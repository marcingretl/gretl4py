import gretl
import numpy as np

gretl.set('verbose', 'off')

try:
  bt = gretl.include('BayTool')
except FileNotFoundError:
  gretl.pkg('install', 'BayTool')
  bt = gretl.include('BayTool')
bt.show_functions()

gretl.set('seed', 1234567)

CASE = 'lm' # lasso, probit, tobit
CASE = 'lasso'

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
  out = gretl.BT_lm(y=y, X='X', prior=prior, opt=opt)
  print(f"Elapsed time in sec:: {gretl.get_accessor('$stopwatch'):.2f} sec.")

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
  out = gretl.BT_lasso(y=dep, X='regr', prior=prior, opt=gretl.Bundle(stdize=0))
  print(f"Elapsed time in sec:: {gretl.get_accessor('$stopwatch'):.2f} sec.")
    
  
  
  
