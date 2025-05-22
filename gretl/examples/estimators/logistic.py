import importlib.resources as resources
import gretl

def run_example():
    data_dir = resources.files('gretl').joinpath('data')

    d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
    d1.set_as_default()

    m1 = gretl.logistic('employ ~ const + prdefl + gnp').fit()
    m2 = gretl.logistic('employ const prdefl gnp').fit()
    m3 = gretl.logistic('1 0 2 3').fit()
    print(m1)
    print(m2)
    print(m3)

    # robust standard errors
    m = gretl.logistic('employ ~ const + prdefl + gnp', robust=True).fit()
    print(m)

    # print covariance matrix
    m = gretl.logistic('employ ~ const + prdefl + gnp', vcv=True).fit()
    print(m)

    # estimate with group fixed effects: requires panel data
    # m = gretl.logistic('employ ~ const + prdefl + gnp', fixed_effects=True).fit()
    # print(m)

if __name__ == "__main__":
    run_example()
