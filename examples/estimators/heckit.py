import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/mroz87.gdt'))
m = gretl.heckit('WW const AX WE CIT ; LFP const WA FAMINC WE').fit()
print(m)