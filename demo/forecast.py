import gretl

d1 = gretl.get_data(gretl.gretl_data_path('ramanathan/data4-1.gdt'))
d1.sample(t1=1, t2=10)

m1 = gretl.ols(formula='1 0 2 3').fit()
print(m1)
fc1 = m1.fcast()
print(fc1)

m2 = gretl.arima(y="price", order=(1,1,1)).fit()
print(m2)
fc2 = m2.fcast()
print(fc2)
