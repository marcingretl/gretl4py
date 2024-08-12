import sys
sys.path.append('./')
sys.path.append('../')
import gretl

fname = gretl.gretl_data_path('misc/abdata.gdt')
gretl.attach_gretl_data(fname)

print('panel, fixed effects')
res = gretl.panel('n ~ 0 + w + k + ys').fit()
print(res)

print('panel, random effects')
res = gretl.panel('n ~ 0 + w + k + ys', random_effects=True).fit()
print(res)
