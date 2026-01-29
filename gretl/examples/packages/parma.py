import gretl

def run_example():
  gretl.set('verbose', 'off')

  try:
    bv = gretl.include('ParMA')
  except FileNotFoundError:
    gretl.pkg('install', 'ParMA')
    bv = gretl.include('ParMA')
  bv.show_functions()
  
  d = gretl.get_data('mroz87.gdt', quiet=True)
  d.new_list(name='X', value='KL6 WA WE HA HE HW MTR UN CIT AX')
  
  modeltype = "probit"
  n_iter = 32 * 1024    # drawings after burn-in
  burn_in = 2048        # burn-in
  
  gretl.set('stopwatch')
  b = gretl.bma_glm(y='LFP', X='X', glm_type=modeltype, ndraw=n_iter, burn=burn_in, parms=gretl.Bundle(seed=1234567))
  print(f"Total execution time: {gretl.get_accessor('$stopwatch'):.2f} sec.")

if __name__ == "__main__":
    run_example()
