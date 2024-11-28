import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('greene/greene22_2.gdt'))
m = gretl.probit('Y ~ const + Z1 + Z2').fit()
print(m)