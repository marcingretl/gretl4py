import gretl
import time
import pandas

ARMAtest = pandas.read_csv('large_arma22.csv')
MyDict = gretl.df2dict(ARMAtest)
armaSpec = tuple((4,0,4))

# arma model via gretl
t0 = time.time()
gmod = gretl.arma(y="y", order=armaSpec, data=MyDict).fit()
t1 = time.time() - t0
print(gmod.results["coeff"])

# arma model via statsmodels
from statsmodels.tsa.arima.model import ARIMA

dta = MyDict["data_array"]
t0 = time.time()

smod = ARIMA(dta, order=armaSpec).fit()
t2 = time.time() - t0
print(smod.params)

print(f"\ngretl = {t1} seconds, statsmodels = {t2} seconds\n")
print(f"ratio = {t1/t2}\n")
