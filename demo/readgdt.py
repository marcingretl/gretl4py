import sys
sys.path.append('./')
sys.path.append('../')
import gretl

mydata = gretl.get_gretl_data("data4-1.gdt")

print("\nOLS example, no options\n")
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata).fit()
print(res)

print("\nOLS example with option\n")
res = gretl.ols('price ~ 0 + sqft + bedrms', data=mydata, robust=True).fit()
print(res)

ldata = gretl.get_gretl_data("greene19_1.gdt")

res = gretl.logit('GRADE ~ 0 GPA TUCE PSI', data=ldata).fit()
print(res)

res = gretl.probit('GRADE ~ 0 GPA TUCE PSI', data=ldata).fit()
print(res)
