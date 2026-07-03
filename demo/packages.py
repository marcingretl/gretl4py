import gretl

def run_example():
    # force libgretl to use the decimal point character
    gretl.set('force_decpoint', 'on')

    # load datasets
    d1 = gretl.get_data('longley.csv')
    d2 = gretl.get_data("FLS_41t72.gdt", frompkg="BMA")

    # we load packages
    CRIT = gretl.include("criteria")
    StrucTiSM = gretl.include("StrucTiSM")
    BMA = gretl.include("BMA")
    extra = gretl.include("extra")

    # let's try out criteria
    print('##################################################')
    print(f'\t\tcriteria')
    print('##################################################')
    CRIT.show_functions()
    # OLS
    m1 = gretl.ols(formula="1 0 2 3 4 5 6 7", simple=True).fit()
    print(m1)
    gretl.genr(name='ess', value=m1.ess)
    gretl.genr(name='k', value=m1.ncoeff)
    res1 = gretl.criteria(n=m1.T, k="k", ess="ess") # arbitrary parameters order!!!!
    print(res1)

    # let's try out BMA
    print('##################################################')
    print(f'\t\tBMA')
    print('##################################################')
    d2.set_as_default()
    OPT = gretl.Bundle()
    OPT['Nrep'] = 10^5
    OPT['model_prior'] = "binomial"
    OPT['Nrank'] = 5
    res2 = gretl.BMA(Y="GDP_growth", X_list='2..13', quiet=True, Options=OPT)
    gretl.BMA_Print(res2)

    # let's try out StrucTiSM
    print('##################################################')
    print(f'\t\tStrucTiSM')
    print('##################################################')
    StrucTiSM.show_functions()
    setup = gretl.STSM_setup(data=d1, y="gnp", epsilon=True, trend=1, slope=1)
    print(setup)
    err = gretl.STSM_estimate(mod=setup)
    if err:
        print(f'STSM_estimate() error: {err}')
    else:
        comps = gretl.STSM_components(mod=setup)
        print(f'components: {comps}')

    # let's try out extra
    print('##################################################')
    print(f'\t\textra')
    print('##################################################')
    m = gretl.Matrix(4, 1 , "normal")
    print(f'\npassed as .numpy_view:\n{gretl.combinations(m.numpy_view, 1)}')
    print(f'\npassed as GretlMatrix:\n{gretl.combinations(m, 1)}')

if __name__ == "__main__":
    run_example()
