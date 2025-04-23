import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))

print(d1.get_accessor("$pd"))

m2 = gretl.arima(y='LRM', order=(2,1,0)).fit()
print(m2)
print("pyt: pd")
print(m2.get_accessor("$pd"))
print("pyt: vcv")
print(m2.get_accessor("$vcv"))
print("pyt: coeff")
print(m2.get_accessor("$coeff"))
print(m2.get_accessor("$parnames"))
print(m2.get_accessor("$xlist"))
print(m2.get_accessor("$uhat"))
