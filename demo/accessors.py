import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('denmark.gdt')))
print("$pd")
print(d1.get_accessor("$pd"))

m2 = gretl.arima(y='LRM', order=(2,1,0)).fit()
print(m2)

for el in ['$vcv', '$coeff', '$parnames', '$xlist', '$uhat']:
    print(f'\n{el}:')
    print(m2.get_accessor(el))
