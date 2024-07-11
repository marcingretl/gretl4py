import sys
sys.path.append("../")
import gretl
from gretl_print import gretl_model_print

''' demo, data still in gdt file '''
# calling arma(1,0,0) [dafault]
print("\nLets call arma_estimate() with default options:")
res = gretl.arma_estimate()
print("\nResults on Python level:")
gretl_model_print(res)

# calling arma(2,1,1,True)
print("\nLets call arma_estimate(2,1,1,True) with default options:")
res = gretl.arma_estimate(2,0,1,True)
print("  object 'res' is of type: " + str(type(res)));
type(res)
print(res)
print("\nResults on Python level:")
gretl_model_print(res)
