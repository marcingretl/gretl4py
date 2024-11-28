import gretl

d = gretl.csv2dict('longley.csv')
print(d.keys())