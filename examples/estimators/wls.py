import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/longley.gdt'))
m = gretl.wls('year employ const prdefl gnp').fit()
print(m)

# robust standard errors
m = gretl.wls('year employ const prdefl gnp', robust=True).fit()
print(m)

# to mix zero weights with positive ones
m = gretl.wls('year employ const prdefl gnp', allow_zeros=True).fit()
print(m)
