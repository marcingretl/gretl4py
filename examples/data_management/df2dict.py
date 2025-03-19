import gretl
import pandas

df = pandas.read_csv('longley.csv')
d = gretl.df2dict(df, with_obs=True)
print(d.keys())