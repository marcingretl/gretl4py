import importlib.resources as resources
import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    # Load data and set as default
    d1 = gretl.get_data(str(data_dir.joinpath('rac3d.gdt')))
    d1.set_as_default()

    # Fit models with different formula styles
    m1 = gretl.negbin('DVISITS ~ const + SEX + AGE').fit()
    m2 = gretl.negbin('DVISITS const SEX AGE').fit()
    m3 = gretl.negbin('13 0 1 2').fit()
    print(m1)
    print(m2)
    print(m3)

    # Use NegBin 1 model
    m = gretl.negbin('DVISITS ~ const + SEX + AGE', model1=True).fit()
    print(m)

    # Robust standard errors
    m = gretl.negbin('DVISITS ~ const + SEX + AGE', robust=True).fit()
    print(m)

    # Force use of OPG method
    m = gretl.negbin('DVISITS ~ const + SEX + AGE', opg=True).fit()
    print(m)

    # Print covariance matrix
    m = gretl.negbin('DVISITS ~ const + SEX + AGE', vcv=True).fit()
    print(m)

    # Print details of iterations
    m = gretl.negbin('DVISITS ~ const + SEX + AGE', verbose=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
