import gretl
d1 = gretl.get_data("bjg")
d2 = gretl.get_data("denmark")

m1 = gretl.arima(y="lg", order=(1,1,1,)).fit()
m2 = gretl.ols("1 0 2").fit()
m3 = gretl.var(4, "IDE IBO LRY LRM", data=d2).fit()

m1.plotfitted()
m1.plotresiduals()

m2.plotfitted()
m2.plotfitted(against='g')
m2.plotresiduals()

m3.plotfitted(eqn=2)
m3.plotresiduals(1)

m3.plotfitted(multiple=True) # m3.plotfitted(multiple=True, backend="Qt6Agg")
m3.plotresiduals(multiple=True)
