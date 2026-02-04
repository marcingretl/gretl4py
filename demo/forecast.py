import gretl

# load datasets
d1 = gretl.get_data('data4-1.gdt')
d2 = gretl.get_data('bjg.gdt')

# set sample
d1.sample(1, 10)

m1 = gretl.ols(formula='1 0 2 3').fit()
print(m1)
m1.fcast2(5, 10)
m1.fcast2(5, 10, "price", out_of_sample=True)
m1.fcast2(5, 10, 3, "price")
m1.fcast2()
#gretl.fc_print(m1.forecasts[-1])

m2 = gretl.arima(y="price", order=(1,1,1)).fit()
print(m2)
m2.fcast2(5, 10, out_of_sample=True)
print(m2.get_accessor("$fcast"))
m2.fcast()
gretl.fc_print(m2.forecasts[-1])

# VAR
print("VAR")
m3 = gretl.var(1, "g lg", data=d2, quiet=True).fit()
#print(m3)
m3.fcast2(stats_only=True)

