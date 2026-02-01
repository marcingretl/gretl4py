import gretl

def run_example():
  # GARCH(1,1) with Normal conditional distribution,
  # no mean regressors.

  gretl.set('verbose', 'off')

  # Import the gig library.
  gretl.include('gig')

  djclose = gretl.get_data('djclose.gdt')
  djclose.new_series('rr = 100*ldiff(djclose)')

  gig_model = gretl.gig_setup('rr')             # equivalent to gig_model = gretl.gig_setup(y='rr')

  gretl.gig_estimate(gig_model)                 # equivalent to gretl.gig_estimate(model=gig_model)


  # Compare with native implementation
  native_model = gretl.garch('1 1 ; rr', nc=True, robust=True).fit()
  print(native_model)

if __name__ == "__main__":
    run_example()
