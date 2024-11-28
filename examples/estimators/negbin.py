import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/rac3d.gdt'))
m = gretl.negbin('DVISITS ~ const + SEX + AGE').fit()
print(m)