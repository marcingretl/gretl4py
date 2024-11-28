import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/longley.gdt'))
m = gretl.ar('1 2 ; employ const prdefl gnp armfrc').fit()
print(m)