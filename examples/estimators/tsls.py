import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/penngrow.gdt'))
m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)').fit()
print(m)