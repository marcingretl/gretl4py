import gretl

def run_example():
    # load a dataset
    d1 = gretl.get_data('longley.csv')
    m = gretl.ar('1 2 ; employ const prdefl gnp armfrc').fit()
    print(m)

    # print covariance matrix
    m = gretl.ar('1 2 ; employ const prdefl gnp armfrc', vcv=True).fit()
    print(m)

    # correlogram
    m.test(type="corrgm")

if __name__ == "__main__":
    run_example()
