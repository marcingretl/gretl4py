import gretl
from numpy import genfromtxt

def run_example():
  # Load CSV data as numpy array, skipping header and first column (assuming index)
  np = genfromtxt(gretl.gretldir + '/data/longley.csv', delimiter=',')[1:, 1:]

  # Convert numpy array to dict of series with gretl helper
  d = gretl.nparray2dict(np)

  # Print keys of resulting dictionary
  print(d.keys())

  # Print dataset basic info
  dset = gretl.get_data(d)
  print(dset)

if __name__ == "__main__":
    run_example()
