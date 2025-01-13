import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/longley.gdt'))
m = gretl.mpols('employ ~ const + prdefl + gnp').fit()
print(m)

# print covariance matrix
m = gretl.mpols('employ ~ const + prdefl + gnp', vcv=True).fit()
print(m)

# do not print auxiliary statistics
m = gretl.mpols('employ ~ const + prdefl + gnp', simple_print=True).fit()
print(m)

# suppress printing of results
m = gretl.mpols('employ ~ const + prdefl + gnp', quiet=True).fit()
print(m)
