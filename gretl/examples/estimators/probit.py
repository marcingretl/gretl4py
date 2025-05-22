import importlib.resources as resources
import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    # Load the dataset explicitly
    d1 = gretl.get_data(str(data_dir.joinpath('greene22_2.gdt')))
    d1.set_as_default()

    m1 = gretl.probit('Y ~ const + Z1 + Z2').fit()
    m2 = gretl.probit('Y const Z1 Z2').fit()
    m3 = gretl.probit('1 0 2 3').fit()
    print(m1)
    print(m2)
    print(m3)

    # robust standard errors
    m = gretl.probit('Y ~ const + Z1 + Z2', robust=True).fit()
    print(m)

    # covariance matrix
    m = gretl.probit('Y ~ const + Z1 + Z2', vcv=True).fit()
    print(m)

    # iteration details
    m = gretl.probit('Y ~ const + Z1 + Z2', verbose=True).fit()
    print(m)

    # p-values instead of slopes
    m = gretl.probit('Y ~ const + Z1 + Z2', p_values=True).fit()
    print(m)

    # pseudo-R^2 variant
    m = gretl.probit('Y ~ const + Z1 + Z2', estrella=True).fit()
    print(m)

    # random effects (commented out)
    # m = gretl.probit('Y ~ const + Z1 + Z2', random_effects=True).fit()
    # print(m)

if __name__ == "__main__":
    run_example()
