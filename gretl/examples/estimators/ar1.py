import importlib.resources as resources
import gretl

def run_example():
    # get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))

    m1 = gretl.ar1('employ ~ const + prdefl + gnp').fit()
    m2 = gretl.ar1('employ const + prdefl + gnp').fit()
    m3 = gretl.ar1('1 0 2 3').fit()
    print(m1)
    print(m2)
    print(m3)

    # Hildreth–Lu procedure
    m = gretl.ar1('employ ~ const + prdefl + gnp', hilu=True).fit()
    print(m)

    # Prais–Winsten estimator
    m = gretl.ar1('employ ~ const + prdefl + gnp', pwe=True).fit()
    print(m)

    # print covariance matrix
    m = gretl.ar1('employ ~ const + prdefl + gnp', vcv=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
