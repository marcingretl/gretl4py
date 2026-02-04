import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

# load datasets
d1 = gretl.get_data('data4-1.gdt')
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
