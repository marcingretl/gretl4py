import gretl

try:
  gretl.include('BMA')
except RuntimeError:
  gretl.pkg('install', 'BMA')
  gretl.include('BMA')
  
d = gretl.get_data('FLS_41t72', frompkg='BMA')
d.new_list('Factors = 2..42')

OPTS = gretl.Bundle()
OPTS['Nrep'] = 3000000
OPTS['burn'] = 33
OPTS['Nrank'] = 20
OPTS['do_joint'] = 'myq'

gretl.set('seed', 1000000)
gretl.set('stopwatch')
RES = gretl.BMA(Y='GDP_growth', X_list='Factors', Options=OPTS)
print(f'Total execution time: {gretl.get_accessor('$stopwatch'):.2f} sec.')
