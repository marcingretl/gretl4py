import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))
print("$pd")
print(d1.get_accessor("$pd"))

m2 = gretl.arima(y='LRM', order=(2,1,0)).fit()
print(m2)

for el in ['$vcv', '$coeff', '$parnames', '$xlist', '$uhat']:
    print(f'\n{el}:')
    print(m2.get_accessor(el))
