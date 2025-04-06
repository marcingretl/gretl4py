import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/longley.gdt'))
m = gretl.mpols('employ ~ const + prdefl + gnp').fit()
print(m)

# print covariance matrix
m = gretl.mpols('employ ~ const + prdefl + gnp', vcv=True).fit()
print(m)

# do not print auxiliary statistics
m = gretl.mpols('employ ~ const + prdefl + gnp', simple_print=True).fit()
print(m)

