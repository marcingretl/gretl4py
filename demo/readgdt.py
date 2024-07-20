import sys
sys.path.append("../")
import gretl
import numpy
from gretl_print import gretl_model_print

mydata = gretl.get_gretl_data("data4-1.gdt")
print("\nOLS example\n")
res = gretl.ols('price ~ 0 + sqft + bedrms', mydata, True)
# print(mydata)
gretl_model_print(res)

mydata = gretl.get_gretl_data("data4-1.gdt")
print("\nOLS example\n")
res = gretl.ols('price ~ 0 + sqft + bedrms', mydata, True)
gretl_model_print(res)

ldata = gretl.get_gretl_data("greene19_1.gdt")
res = gretl.logit('GRADE ~ 0 GPA TUCE PSI', ldata, True)
gretl_model_print(res)
res = gretl.probit('GRADE ~ 0 GPA TUCE PSI', ldata, True)
gretl_model_print(res)
