import gretl
import pandas

df = pandas.read_csv('longley.csv')
d = gretl.df2dict(df, withobs=True)
print(d.keys())