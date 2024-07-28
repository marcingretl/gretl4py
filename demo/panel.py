import sys
sys.path.append("../")
import gretl
from gretl4py_addons import gretl_data_path, pretty_print

fname = gretl_data_path('misc/abdata.gdt')
gretl.attach_gretl_data(fname)

print('panel, fixed effects')
res = gretl.panel('n ~ 0 + w + k + ys')
pretty_print(res)

print('panel, random effects')
res = gretl.panel('n ~ 0 + w + k + ys', random_effects=True)
pretty_print(res)
