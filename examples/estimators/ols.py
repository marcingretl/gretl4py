import gretl

gretl.attach_gretl_data(gretl.gretl_data_path('misc/longley.gdt'))

# Alternative ways of calling OLS model
m1 = gretl.ols('employ ~ const + prdefl + gnp').fit()
m2 = gretl.ols('employ const prdefl gnp').fit()
m3 = gretl.ols('1 0 2 3').fit()
print(m1)
print(m2)
print(m3)

# print covariance matrix
m = gretl.ols('employ const prdefl gnp', vcv=True).fit()
print(m)

# robust standard errors
m = gretl.ols('employ const prdefl gnp', robust=True).fit()
print(m)

# ... and jackknife option
m = gretl.ols('employ const prdefl gnp',
              robust=True, jackknife=True).fit()
print(m)

# simple-print option
m = gretl.ols('employ const prdefl gnp', simple_print=True).fit()
print(m)

# suppress printing of results
m = gretl.ols('employ const prdefl gnp', quiet=True).fit()
print(m)

# print an ANOVA table
m = gretl.ols('employ const prdefl gnp', anova=True).fit()
print(m)

# no-df-corr option
m = gretl.ols('employ const prdefl gnp', no_df_corr=True).fit()
print(m)
