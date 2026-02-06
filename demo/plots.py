import gretl
d=gretl.get_data("bjg")

m1 = gretl.arima(y="lg", order=(1,1,1,)).fit()
m2 = gretl.ols("1 0 2").fit()
m3 = gretl.var(4, "lg g").fit()

m1.plotfitted()
m1.plotresiduals()
 
m2.plotfitted()
m2.plotresiduals()

m3.plotfitted(eqn=2)
m3.plotresiduals(eqn=1)
