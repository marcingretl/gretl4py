import gretl
import time

MyDict = gretl.get_data('large_arma22.csv')
arimaSpec = tuple((4,0,4))
res = gretl.Matrix(9,2)

# arma model via gretl
t0 = time.time()
gmod = gretl.arima(depvar="y", order=arimaSpec, data=MyDict).fit()
t1 = time.time() - t0
res.numpy_view[:, 0] = gmod.coeff.numpy_view

# arma model via statsmodels
from statsmodels.tsa.arima.model import ARIMA

dta = MyDict.to_dict()["data_array"]
t0 = time.time()

smod = ARIMA(dta, order=arimaSpec).fit()
t2 = time.time() - t0
res.numpy_view[:, 1] = smod.params[:9]

res.rnameset(gmod.parnames)
res.cnameset(['gretl4py', 'statsmodels'])
print(f'\n{res}')

print(f"gretl = {t1} seconds, statsmodels = {t2} seconds\n")
print(f"ratio = {t1/t2}\n")
