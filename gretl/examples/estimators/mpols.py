import importlib.resources as resources
import gretl

def run_example():
    # get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
    d1.set_as_default()

    m = gretl.mpols('employ ~ const + prdefl + gnp').fit()
    print(m)

    # print covariance matrix
    m = gretl.mpols('employ ~ const + prdefl + gnp', vcv=True).fit()
    print(m)

    # do not print auxiliary statistics
    m = gretl.mpols('employ ~ const + prdefl + gnp', simple_print=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
