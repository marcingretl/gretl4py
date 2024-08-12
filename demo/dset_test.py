import sys
sys.path.append('./')
sys.path.append('../')
import gretl
import pandas

d97 = gretl.csv2dict('data9-7.csv', 4)

print("Loading global dataset:")
gretl.attach_data(d97)

print("\nOLS example using global dataset:\n")
res = gretl.ols('QNC ~ 0 + PRICE + INCOME').fit()
print(res)

print("\nOLS example passing dataset as dict:\n")
res = gretl.ols('QNC ~ 0 + PRICE + INCOME', data=d97).fit()
print(res)

dframe = pandas.read_csv('data9-7.csv')
MyDict = gretl.df2dict(dframe)
res = gretl.ols('QNC ~ 0 + PRICE + INCOME', data=MyDict).fit()
print(res)
