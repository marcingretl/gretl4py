import gretl
import pandas

df = pandas.read_csv('longley.csv')
s = pandas.Series(df['employ'].to_list(), index=df['obs'].to_list()) 
d = gretl.series2dict(s)
print(d.keys())