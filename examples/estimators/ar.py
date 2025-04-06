import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/longley.gdt'))
m = gretl.ar('1 2 ; employ const prdefl gnp armfrc').fit()
print(m)

# print covariance matrix
m = gretl.ar('1 2 ; employ const prdefl gnp armfrc', vcv=True).fit()
print(m)
