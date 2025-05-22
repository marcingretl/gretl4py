import importlib.resources as resources
import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    # Load dataset and set it as default
    d1 = gretl.get_data(str(data_dir.joinpath('rac3d.gdt')))
    d1.set_as_default()

    # Basic model forms
    m1 = gretl.poisson('DVISITS ~ const + SEX + AGE').fit()
    m2 = gretl.poisson('DVISITS const SEX AGE').fit()
    m3 = gretl.poisson('13 0 1 2').fit()
    print(m1)
    print(m2)
    print(m3)

    # Robust standard errors
    m = gretl.poisson('DVISITS ~ const + SEX + AGE', robust=True).fit()
    print(m)

    # Covariance matrix
    m = gretl.poisson('DVISITS ~ const + SEX + AGE', vcv=True).fit()
    print(m)

    # Iteration details
    m = gretl.poisson('DVISITS ~ const + SEX + AGE', verbose=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
