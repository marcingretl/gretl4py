import sys
sys.path.append("../")

import gretl

def print_model(RES):
    '''Function prints results from libgretl'''
    ASHEDLINE = "---------------------------------------------------------"
    if isinstance(RES, dict) is not True:
        print("I can print only dictionary!!!\n")
        return -1
    else:
        print("\t\t coefficient\t std. error\n  " + ASHEDLINE)
        coeff_names = RES["coeff"].keys()
        for variable in coeff_names:
            print(f'  {variable:s}\t\t {RES["coeff"][variable]:f}\t {RES["sderr"][variable]:f}')
        print("\n")

# calling arma(1,0,0) [dafault]
print("\nLets call arma_estimate() with default options:")
res = gretl.arma_estimate()
print("\nResults on Python level:")
print_model(res)

# calling arma(2,1,1,True)
print("\nLets call arma_estimate(2,1,1,True) with default options:")
res = gretl.arma_estimate(2,0,1,True)
print("  object 'res' is of type: " + str(type(res)));
type(res)
print(res)
print("\nResults on Python level:")
print_model(res)
