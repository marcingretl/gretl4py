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
  mod = gretl.BVAR_setup('dep', 4, 'conj')                                    # equivalent to mod = gretl.BVAR_setup(y='dep', lag=4, type='conj')
  
  # Step 2 - prior customization
  gretl.BVAR_sigma_prior(mod, 'AR')                                           # equivalent to gretl.BVAR_sigma_prior(mod=mod, prior_type='AR')
  gretl.BVAR_alpha_prior(mod, 'Minn', gretl.Bundle(p_mean=0))                 # equivalent to gretl.BVAR_alpha_prior(mod=mod, prior_type='Minn', prior_opt=gretl.Bundle(p_mean=0))
  
  # Step 3 - posterior simulation
  gretl.set('seed', 1234)
  res = gretl.BVAR_posterior(mod, 'all', gretl.Bundle(iter=10000))            # equivalent to res = gretl.BVAR_posterior(mod=mod, wat='all', opt=gretl.Bundle(iter=10000))
  res.print(tree=True)

if __name__ == "__main__":
    run_example()
