import gretl

def run_example():
    # get path object for the data directory inside the gretl package
    d1 = gretl.get_data('longley.csv')

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
