import importlib.resources as resources
import gretl

def run_example():
    data_dir = resources.files('gretl').joinpath('data')
    d1 = gretl.get_data(str(data_dir.joinpath('b-g.gdt')))
    d1.set_as_default()

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
