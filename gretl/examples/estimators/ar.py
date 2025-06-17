import importlib.resources as resources
import gretl

def run_example():
    # get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
    m = gretl.ar('1 2 ; employ const prdefl gnp armfrc').fit()
    print(m)

    # print covariance matrix
    m = gretl.ar('1 2 ; employ const prdefl gnp armfrc', vcv=True).fit()
    print(m)

    # correlogram
    m.test(type="corrgm")

if __name__ == "__main__":
    run_example()
