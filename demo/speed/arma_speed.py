import gretl
import time

MyDict = gretl.csv2dict('large_arma22.csv', "1")
arimaSpec = tuple((4,0,4))

# arma model via gretl
t0 = time.time()
gmod = gretl.arima(y="y", order=arimaSpec, data=MyDict).fit()
t1 = time.time() - t0
print(gmod.coeff)

# arma model via statsmodels
from statsmodels.tsa.arima.model import ARIMA

dta = MyDict["data_array"]
t0 = time.time()

smod = ARIMA(dta, order=arimaSpec).fit()
t2 = time.time() - t0
print(smod.params)

print(f"\ngretl = {t1} seconds, statsmodels = {t2} seconds\n")
print(f"ratio = {t1/t2}\n")
