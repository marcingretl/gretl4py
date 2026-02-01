import gretl

def run_example():
    # force libgretl to use the decimal point character
    gretl.set('force_decpoint', 'on')

    # load datasets
    d = gretl.get_data('longley.csv')

    # load the package
    CRIT = gretl.include('criteria')

    # display the function signatures provided by the 'criteria' package
    CRIT.show_functions()

    # OLS
    m = gretl.ols(formula="1 0 2 3 4 5 6 7", simple=True).fit()
    print(m)

    gretl.genr(name='ess', value=m.ess)
    gretl.genr(name='k', value=m.ncoeff)
    gretl.genr(name='n', value=m.T)
    
    # call the 'criteria' function in different ways

    # positional arguments only
    res = gretl.criteria('ess', m.T, 'k')
    print(res)

    # keyword arguments in arbitrary order
    res2 = gretl.criteria(n=m.T, k='k', ess='ess')
    print(res2)

    # mix of positional and keyword arguments
    res3 = gretl.criteria('ess', k='k', n=m.T)
    print(res3)

if __name__ == "__main__":
    run_example()
