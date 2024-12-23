import gretl
import pandas

dframe = pandas.read_csv('data9-7.csv')
MyDict = gretl.df2dict(dframe, 4, True)

# arma model
armamod = gretl.arima(y='QNC', order=(1,0,1), data=MyDict).fit()
print(armamod)

# ols with r-style formula
olsmod = gretl.ols('QNC ~ const + PRICE + INCOME', data=MyDict).fit()
print(olsmod)

# probit with native printout
probmod = gretl.probit('WINTER ~ const + PRICE + INCOME', data=MyDict).fit()
print(probmod)

