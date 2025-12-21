import gretl
import pandas

# Read CSV into a pandas DataFrame
df = pandas.read_csv('longley.csv', index_col="obs", parse_dates=True, date_format="%Y")

# Create a pandas Series for the 'employ' column, indexed by 'obs'
s = df['employ']

# Convert the pandas Series into a dictionary compatible with gretl
d = gretl.series2dict(s)

# Print the keys of the resulting dictionary
print(d.keys())

# Print dataset basic info
dset = gretl.get_data(d)
print(dset)
