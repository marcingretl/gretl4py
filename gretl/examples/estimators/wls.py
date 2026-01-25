import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    d1 = gretl.get_data('longley.csv')

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
