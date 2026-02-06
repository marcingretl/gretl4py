import gretl

# load datasets
d1 = gretl.get_data('data4-1.gdt')
d2 = gretl.get_data('bjg.gdt')

# set sample
d1.sample(1, 10)
d2.sample(1, d2.n-4)

m1 = gretl.ols(formula='1 0 2 3').fit()
#print(m1)
m1.fcast2(5, 10)
m1.fcast2(out_of_sample=True)
m1.fcast2(5, 10, 3, "price", recursive=True)
m1.fcast2()
#gretl.fc_print(m1.forecasts[-1])

# ARIMA
m2 = gretl.arima(y="price", order=(1,1,1)).fit()
print(m2)
m2.fcast2(5, 10)
m2.fcast2(out_of_sample=True)
print(m2.get_accessor("$fcast"))
#m2.fcast()
#gretl.fc_print(m2.forecasts[-1])


# VAR
print("VAR")
m4 = gretl.var(1, "g lg", data=d2, quiet=True).fit()
#print(m4)
m4.fcast2(d2.n-3, d2.n, out_of_sample=True)
