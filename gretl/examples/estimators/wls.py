import importlib.resources as resources
import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    # Load the dataset explicitly and set it as default
    d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
    d1.set_as_default()

    m = gretl.wls('year employ const prdefl gnp').fit()
    print(m)

    # print covariance matrix
    m = gretl.wls('year employ const prdefl gnp', vcv=True).fit()
    print(m)

    # robust standard errors
    m = gretl.wls('year employ const prdefl gnp', robust=True).fit()
    print(m)

    # to mix zero weights with positive ones
    m = gretl.wls('year employ const prdefl gnp', allow_zeros=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
