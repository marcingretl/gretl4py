import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/kennan.gdt'))
m = gretl.duration('Ti ~ const + Prod').fit()
print(m)