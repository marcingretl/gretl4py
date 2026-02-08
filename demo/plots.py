import gretl
d1 = gretl.get_data("bjg")
d2 = gretl.get_data("denmark")

m1 = gretl.arima(y="lg", order=(1,1,1,)).fit()
m2 = gretl.ols("1 0 2").fit()
m3 = gretl.var(4, "IDE IBO LRY LRM", data=d2).fit()

m1.plot('fitted', against='lg')
m1.plot('residuals')

m2.plot('fitted')
m2.plot('residuals', against='g')

m3.plot('fitted', eqn=2)
m3.plot('residuals', 1)

m3.plot('fitted', multiple=True) # m3.plotfitted(multiple=True, backend="Qt6Agg")
m3.plot('residuals', multiple=True)
