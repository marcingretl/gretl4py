import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/rac3d.gdt'))
m = gretl.poisson('DVISITS ~ const + SEX + AGE').fit()
print(m)