import gretl
import matplotlib.pyplot as plt

d1 = gretl.get_data("bjg")
d2 = gretl.get_data("denmark")

m1 = gretl.arima(depvar="lg", order=(1,1,1,)).fit()
m2 = gretl.ols("1 0 2").fit()
m3 = gretl.var(4, "IDE IBO LRY LRM", data=d2).fit()

m1.plot.fitted_actual('lg')
m1.plot.residuals()

m2.plot.fitted_actual()
m2.plot.residuals('g')

#m3.plot.fitted_actual(eqn=2)
m3.plot.residuals(1)

#m3.plot.fitted_actual(multiple=True) # m3.plot(kind="fitted", multiple=True, backend="Qt6Agg")
m3.plot.residuals()
plt.show()
