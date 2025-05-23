import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

# load datasets
d1 = gretl.get_data(str(data_dir.joinpath('data4-1.gdt')))
d1.sample(t1=1, t2=10)

m1 = gretl.ols(formula='1 0 2 3').fit()
print(m1)
m1.fcast()
gretl.fc_print(m1.forecasts[-1])

m2 = gretl.arima(y="price", order=(1,1,1)).fit()
print(m2)
m2.fcast()
gretl.fc_print(m2.forecasts[-1])
