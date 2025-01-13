import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/longley.gdt'))
m1 = gretl.lad('employ ~ const + prdefl + gnp').fit()
m2 = gretl.lad('employ const prdefl gnp').fit()
m3 = gretl.lad('1 0 2 3').fit()
print(m1)
print(m2)
print(m3)
