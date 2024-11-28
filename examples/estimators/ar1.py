import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/longley.gdt'))
m = gretl.ar1('employ ~ const + prdefl + gnp').fit()
print(m)