import gretl
from numpy import genfromtxt

np = genfromtxt('longley.csv', delimiter=',')[1:,1:]
d = gretl.nparray2dict(np)
print(d.keys())