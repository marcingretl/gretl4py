import importlib.resources as resources
import gretl

def run_example():
    # load a dataset
    d1 = gretl.get_data('greene22_2.gdt')    
    d1.new_series('rat = Z8 - 1')
    d1.discrete('rat')

    m1 = gretl.probit('rat ~ const + Z1 + Z2').fit()
    m2 = gretl.probit('rat const Z1 Z2').fit()
    m3 = gretl.probit(str(d1.varnames.index('rat')) + ' 0 2 3').fit()
    print(m1)
    print(m2)
    print(m3)

    # robust standard errors
    m = gretl.probit('rat ~ const + Z1 + Z2', robust=True).fit()
    print(m)

    # covariance matrix
    m = gretl.probit('rat ~ const + Z1 + Z2', vcv=True).fit()
    print(m)

    # iteration details
    m = gretl.probit('rat ~ const + Z1 + Z2', verbose=True).fit()
    print(m)

    # p-values instead of slopes
    m = gretl.probit('rat ~ const + Z1 + Z2', p_values=True).fit()
    print(m)

    # pseudo-R^2 variant
    m = gretl.probit('rat ~ const + Z1 + Z2', estrella=True).fit()
    print(m)

    # random effects probit
    d2 = gretl.get_data('union_wooldridge.gdt')
    m = gretl.probit('union const black married educ dummify(year)', data=d2, random_effects=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
