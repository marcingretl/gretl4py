import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/b-g.gdt'))
m = gretl.garch('1 1 ; Y').fit()
print(m)