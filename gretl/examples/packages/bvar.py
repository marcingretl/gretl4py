import gretl

def run_example():
  gretl.set('verbose', 'off')

  try:
    bv = gretl.include('BVAR')
  except FileNotFoundError:
    gretl.pkg('install', 'BVAR')
    bv = gretl.include('BVAR')
  bv.show_functions()
  
  d = gretl.get_data('koopkoro.gdt', frompkg='BVAR')
  d.new_list(name='dep', value='1 2 3')
  
  # Step 1 - BVAR_setup
  model = gretl.BVAR_setup(y='dep', lag=4, type='conj')
  
  # Step 2 - prior customization
  OPT = gretl.Bundle()
  OPT['p_mean'] = 0
  gretl.BVAR_sigma_prior(mod=model, prior_type='AR')
  gretl.BVAR_alpha_prior(mod=model, prior_type='Minn', prior_opt=OPT)
  
  # Step 3 - posterior simulation
  OPT = gretl.Bundle()
  OPT['iter'] = 10000
  gretl.set('seed', 1234)
  res = gretl.BVAR_posterior(mod=model, wat='all', opt=OPT)
  res.print(tree=True)

if __name__ == "__main__":
    run_example()
