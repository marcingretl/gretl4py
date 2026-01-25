import gretl

def run_example():
    # get path object for the data directory inside the gretl package
    d1 = gretl.get_data('mroz87.gdt')

    formula = 'WW const AX WE CIT ; LFP const WA FAMINC WE'

    m = gretl.heckit(formula).fit()
    print(m)

    # perform two-step estimation
    m = gretl.heckit(formula, two_step=True).fit()
    print(m)

    # print covariance matrix
    m = gretl.heckit(formula, vcv=True).fit()
    print(m)

    # force use of the OPG method
    m = gretl.heckit(formula, opg=True).fit()
    print(m)

    # robust standard errors
    m = gretl.heckit(formula, robust=True).fit()
    print(m)

    # print extra output
    m = gretl.heckit(formula, verbose=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
