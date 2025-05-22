import gretl
import pandas

# Read CSV into pandas DataFrame
df = pandas.read_csv('longley.csv')

# Convert DataFrame to dict of series, preserving observation labels
d = gretl.df2dict(df, with_obs=True)

# Print keys of the resulting dictionary
print(d.keys())
