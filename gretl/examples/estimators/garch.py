import gretl

def run_example():
    # get path object for the data directory inside the gretl package
    d1 = gretl.get_data('b-g.gdt')

    spec = '1 1 ; Y'

    m = gretl.garch(spec).fit()
    print(m)

    # robust standard errors
    m = gretl.garch(spec, robust=True).fit()
    print(m)

    # print details of iterations
    m = gretl.garch(spec, verbose=True).fit()
    print(m)

    # print covariance matrix
    m = gretl.garch(spec, vcv=True).fit()
    print(m)

    # model without constant
    m = gretl.garch(spec, nc=True).fit()
    print(m)

    # standardized residuals
    m = gretl.garch(spec, stdresid=True).fit()
    print(m)

    # use Fiorentini, Calzolari, Panattoni algorithm
    m = gretl.garch(spec, fcp=True).fit()
    print(m)

    # initial variance parameters from ARMA
    m = gretl.garch(spec, arma_init=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
