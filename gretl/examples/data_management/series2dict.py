import gretl
import pandas

# Read CSV into a pandas DataFrame
df = pandas.read_csv('longley.csv')

# Create a pandas Series for the 'employ' column, indexed by 'obs'
s = pandas.Series(df['employ'].to_list(), index=df['obs'].to_list())

# Convert the pandas Series into a dictionary compatible with gretl
d = gretl.series2dict(s)

print(d.keys())
# Print the keys of the resulting dictionary
