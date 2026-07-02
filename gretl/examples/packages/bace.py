import gretl

def run_example():
  # the BACE package; if not found, install it from SourceForge
  try:
    bc = gretl.include('BACE')
  except FileNotFoundError:
    gretl.pkg('install', 'BACE')
    bc = gretl.include('BACE')
  bc.show_functions()

  d = gretl.get_data('nels.gdt', frompkg='BACE', quiet=True)

  gretl.set('seed', 1000000)
  gretl.set('stopwatch')
  res = gretl.BACE('psechoice', '2..14', gretl.Bundle(model_type="probit")) # equivalent to res = gretl.BACE(Y='psechoice', X_list='2..14', Options=gretl.Bundle(model_type="probit"))
  print(f"Total execution time: {gretl.get_accessor('$stopwatch'):.2f} sec.")

if __name__ == "__main__":
    run_example()
