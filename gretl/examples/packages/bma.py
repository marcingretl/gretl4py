import gretl

def run_example():
  # the BMA package; if not found, install it from SourceForge
  try:
    gretl.include('BMA')
  except FileNotFoundError:
    gretl.pkg('install', 'BMA')
    gretl.include('BMA')
  
  # load the dataset bundled with the BMA package and
  # define a list of potential GDP growth factors
  d = gretl.get_data('FLS_41t72', frompkg='BMA')
  d.new_list(name='Factors', value='2..42')

  # set MC3 options
  OPTS = gretl.Bundle()
  OPTS['Nrep'] = 3000000
  OPTS['burn'] = 33
  OPTS['Nrank'] = 20
  OPTS['do_joint'] = 'myq'

  # run BMA
  gretl.set('seed', 1000000)
  gretl.set('stopwatch')
  RES = gretl.BMA(Y='GDP_growth', X_list='Factors', Options=OPTS)
  print(f"Total execution time: {gretl.get_accessor('$stopwatch'):.2f} sec.")

if __name__ == "__main__":
    run_example()
