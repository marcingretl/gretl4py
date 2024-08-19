import gretl

data97 = gretl.csv2dict('data9-7.csv', 4)

mdl = gretl.arma(y="QNC", order=(1,1,1), data=data97).fit()
results = mdl.results
# first element should be NaN
print(results['uhat'])
