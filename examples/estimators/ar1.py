import gretl

gretl.attach_gretl_data(
	gretl.gretl_data_path('misc/longley.gdt'))
m1 = gretl.ar1('employ ~ const + prdefl + gnp').fit()
m2 = gretl.ar1('employ const + prdefl + gnp').fit()
m3 = gretl.ar1('1 0 2 3').fit()
print(m1)
print(m2)
print(m3)

# Hildreth–Lu procedure
m = gretl.ar1('employ ~ const + prdefl + gnp', hilu=True).fit()
print(m)

# Prais–Winsten estimator
m = gretl.ar1('employ ~ const + prdefl + gnp', pwe=True).fit()
print(m)

# print covariance matrix
m = gretl.ar1('employ ~ const + prdefl + gnp', vcv=True).fit()
print(m)

# don't print anything
m = gretl.ar1('employ ~ const + prdefl + gnp', quiet=True).fit()
print(m)