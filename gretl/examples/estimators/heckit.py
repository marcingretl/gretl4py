import importlib.resources as resources
import gretl

def run_example():
    data_dir = resources.files('gretl').joinpath('data')
    d1 = gretl.get_data(str(data_dir.joinpath('mroz87.gdt')))
    d1.set_as_default()

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
