import gretl

def run_example():
    # load a dataset
    d1 = gretl.get_data('penngrow.gdt')

    # Basic models
    m1 = gretl.panel('Y ~ const + Y(-1) + X').fit()
    m2 = gretl.panel('Y const Y(-1) X').fit()
    print(m1)
    print(m2)

    # Covariance matrix
    m = gretl.panel('Y ~ const + Y(-1) + X', vcv=True).fit()
    print(m)

    # Group fixed effects
    m = gretl.panel('Y ~ const + Y(-1) + X', fixed_effects=True).fit()
    print(m)

    # Random effects / GLS model
    m = gretl.panel('Y ~ const + Y(-1) + X', random_effects=True).fit()
    print(m)

    # Nerlove transformation (commented out)
    # m = gretl.panel('Y ~ const + Y(-1) + X', nerlove=True).fit()
    # print(m)

    # Pooled OLS
    m = gretl.panel('Y ~ const + Y(-1) + X', pooled=True).fit()
    print(m)

    # Between-groups model
    m = gretl.panel('Y ~ const + Y(-1) + X', between=True).fit()
    print(m)

    # Robust standard errors
    m = gretl.panel('Y ~ const + Y(-1) + X', robust=True).fit()
    print(m)

    # Time dummy variables
    m = gretl.panel('Y ~ const + Y(-1) + X', time_dummies=True).fit()
    print(m)

    # Weighted least squares
    m = gretl.panel('Y ~ const + Y(-1) + X', unit_weights=True).fit()
    print(m)

    # Iterative estimation
    m = gretl.panel('Y ~ const + Y(-1) + X', iterate=True).fit()
    print(m)

    # Hausman test via matrix difference
    m = gretl.panel('Y ~ const + Y(-1) + X', matrix_diff=True).fit()
    print(m)

    # Verbose output
    m = gretl.panel('Y ~ const + Y(-1) + X', verbose=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
