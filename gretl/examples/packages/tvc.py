import gretl
import numpy as np

def run_example():
  try:
    tvc = gretl.include('TVC')
  except FileNotFoundError:
    gretl.pkg('install', 'TVC')
    tvc = gretl.include('TVC')
  tvc.show_functions()
  
  d = gretl.get_data('OkunGer.gdt', frompkg='TVC', quiet=True)
  d.new_list(name='L', value='gGDP intercept')
   
  P = gretl.TVCestimate('dU', 'L', plt=False)
  #P = gretl.TVCestimate('dU', 'L', ratios=np.array([1, 0.8]), plt=False)
  gretl.set('echo', 'on')

if __name__ == "__main__":
    run_example()
