import gretl
import pandas

def run_example():
  # Read CSV into a pandas DataFrame
  df = pandas.read_csv(gretl.gretldir + '/data/longley.csv', index_col="obs", parse_dates=True, date_format="%Y")

  # Create a pandas Series for the 'employ' column, indexed by 'obs'
  s = df['employ']

  # Convert the pandas Series into a dictionary compatible with gretl
  d = gretl.series2dict(s)

  # Print the keys of the resulting dictionary
  print(d.keys())

  # Print dataset basic info
  dset = gretl.get_data(d)
  print(dset)

if __name__ == "__main__":
    run_example()
