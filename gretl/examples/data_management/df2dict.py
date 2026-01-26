import gretl
import pandas

def run_example():
  # Read CSV into pandas DataFrame
  df = pandas.read_csv(gretl.gretldir + '/data/longley.csv')

  # Convert DataFrame to dict of series, preserving observation labels
  d = gretl.df2dict(df, with_obs=True)

  # Print keys of the resulting dictionary
  print(d.keys())

  # Print dataset basic info
  dset = gretl.get_data(d)
  print(dset)

if __name__ == "__main__":
    run_example()
