import gretl
import numpy
from gretl4py_addons import gretl_data_path
from gretl4py_classes import *

fname = gretl_data_path('misc/abdata.gdt')
gretl.attach_gretl_data(fname)

print('panel, fixed effects')
res = gretl.panel('n ~ 0 + w + k + ys').fit()
print(res)

print('panel, random effects')
res = gretl.panel('n ~ 0 + w + k + ys', random_effects=True).fit()
print(res)
