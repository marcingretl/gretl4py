import sys
import gretl

fname = gretl.gretl_data_path('ramanathan/data4-10.gdt')
gretl.attach_gretl_data(fname)

print("\nOLS example, no options\n")
res = gretl.ols('ENROLL ~ 0 + CATHOL + INCOME').fit()
print(res)

gretl.sample()
gretl.sample(t1=5, t2=35)

