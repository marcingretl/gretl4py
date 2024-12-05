import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/bjg.gdt'))
m = gretl.arima(y='lg', order=(1,1,1)).fit()
print(m)