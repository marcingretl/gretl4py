import gretl

data97 = gretl.csv2dict('data9-7.csv', 4)

OLS1 = gretl.ols('QNC ~ const + PRICE + INCOME', data=data97, print=False)

# OLS

print("Print model before computing")
print(OLS1)
print("\n\n\n")

print("Fit model")
OLS1.fit()

print("Print result")
print(OLS1)
print("\n\n\n")

# ARMA

ARMA1 = gretl.arma(y="QNC", order=(1,1,1), data=data97, print=True)
ARMA1.fit()
print(ARMA1)
