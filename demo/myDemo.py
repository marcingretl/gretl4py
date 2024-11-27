import gretl

data97 = gretl.csv2dict('data9-7.csv', 4)

# ARMA
ARMA1 = gretl.arma(y="QNC", order=(1,1,1), data=data97)
ARMA1.fit()
print(ARMA1)
print(ARMA1.yhat())
