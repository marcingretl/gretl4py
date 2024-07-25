import sys
sys.path.append("../")
import gretl
from gretl4py_addons import gretl_model_print

# load bjg.csv using libgretl and set as global DSET
gretl.attach_gretl_data('bjg.csv')

print('seasonal arma (0 1 2, 0 1 1)\n')
res = gretl.arma(y='lg', order=(0,1,2), seasonal=(0,1,1), nc=True, print=True)
print('\n\at python level:')
gretl_model_print(res)
