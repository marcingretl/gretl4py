import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/penngrow.gdt'))
m = gretl.panel('Y ~ const + Y(-1) + X').fit()
print(m)