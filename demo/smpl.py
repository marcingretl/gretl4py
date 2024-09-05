import gretl

fname = gretl.gretl_data_path('ramanathan/data4-10.gdt')
gretl.attach_gretl_data(fname)

res = gretl.ols('ENROLL ~ 0 + CATHOL + INCOME').fit()
print(res)

gretl.sample(t1=5, t2=35)

res = gretl.ols('ENROLL ~ 0 + CATHOL + INCOME').fit()
print(res)

gretl.sample(restriction='REGION>1', replace=True)
res = gretl.ols('ENROLL ~ 0 + CATHOL + INCOME').fit()
print(res)
