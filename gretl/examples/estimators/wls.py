import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("longley.csv")

    # Estimate weighted least squares model.
    m = gretl.wls("year employ const prdefl gnp").fit()
    print(m)

    # Print covariance matrix.
    m = gretl.wls("year employ const prdefl gnp", vcv=True).fit()
    print(m)

    # Compute robust covariance matrix.
    m = gretl.wls("year employ const prdefl gnp", robust=True).fit()
    print(m)

    # Allow zero weights in estimation.
    m = gretl.wls("year employ const prdefl gnp", allow_zeros=True).fit()
    print(m)


if __name__ == "__main__":
    run_example()
