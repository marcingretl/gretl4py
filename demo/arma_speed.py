import sys
sys.path.append("../")
import time

import gretl
import pandas
# from gretl_print import gretl_model_print
from df2dict import df2dict

ARMAtest = pandas.read_csv('large_arma22.csv')
MyDict = df2dict(ARMAtest)

# arma model via gretl
t0 = time.time()
gmod = gretl.arma(data=MyDict, y="y", p=2, d=0, q=2)
t1 = time.time() - t0
print(gmod["coeff"])

# arma model via statsmodels
from statsmodels.tsa.arima.model import ARIMA

dta = MyDict["data_array"]
t0 = time.time()

smod = ARIMA(dta, order=(2,0,2)).fit()
t2 = time.time() - t0
print(smod.params)

print(f"\ngretl = {t1} seconds, statsmodels = {t2} seconds\n")
print(f"ratio = {t1/t2}\n")
