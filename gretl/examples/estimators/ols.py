import importlib.resources as resources
import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    # Load dataset and set as default
    d1 = gretl.get_data(str(data_dir.joinpath('longley.csv')))
    d1.set_as_default()

    # Alternative ways of calling OLS model
    m1 = gretl.ols('employ ~ const + prdefl + gnp').fit()
    m2 = gretl.ols('employ const prdefl gnp').fit()
    m3 = gretl.ols('1 0 2 3').fit()
    print(m1)
    print(m2)
    print(m3)

    # Print covariance matrix
    m = gretl.ols('employ const prdefl gnp', vcv=True).fit()
    print(m)

    # Robust standard errors
    m = gretl.ols('employ const prdefl gnp', robust=True).fit()
    print(m)

    # Robust + jackknife standard errors
    m = gretl.ols('employ const prdefl gnp', robust=True, jackknife=True).fit()
    print(m)

    # Simple print output
    m = gretl.ols('employ const prdefl gnp', simple_print=True).fit()
    print(m)

    # Print ANOVA table
    m = gretl.ols('employ const prdefl gnp', anova=True).fit()
    print(m)

    # No degrees-of-freedom correction
    m = gretl.ols('employ const prdefl gnp', no_df_corr=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
