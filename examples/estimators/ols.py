import gretl

file = gretl.gretl_data_path('misc/banks91.gdt')
gretl.attach_gretl_data(file)

# Alternative ways of calling OLS model
m1 = gretl.ols('VC ~ const + Q1 + Q2 + P1 + P2 + P3').fit()
m2 = gretl.ols('VC const Q1 Q2 P1 P2 P3').fit()
m3 = gretl.ols('1 0 2 3 4 5 6').fit()
print(m1)
print(m2)
print(m3)

# robust option
m = gretl.ols('VC const Q1 Q2 P1 P2 P3', robust=True).fit()
print(m)

# ... and jackknife option
m = gretl.ols('VC const Q1 Q2 P1 P2 P3',
              robust=True, jackknife=True).fit()
print(m)

# simple-print option
m = gretl.ols('VC const Q1 Q2 P1 P2 P3', simple_print=True).fit()
print(m)
print("Oups, wrong result!!!")

# no-df-corr option
m = gretl.ols('VC const Q1 Q2 P1 P2 P3', no_df_corr=True).fit()
print(m)
