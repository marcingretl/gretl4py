import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("longley.csv")

    # Different ways to specify AR(1) model.
    m1 = gretl.ar1("employ ~ const + prdefl + gnp").fit()
    m2 = gretl.ar1("employ const + prdefl + gnp").fit()
    m3 = gretl.ar1("1 0 2 3").fit()

    print(m1)
    print(m2)
    print(m3)

    # Hildreth–Lu estimator.
    m = gretl.ar1(
        "employ ~ const + prdefl + gnp",
        hilu=True
    ).fit()
    print(m)

    # Prais–Winsten estimator.
    m = gretl.ar1(
        "employ ~ const + prdefl + gnp",
        pwe=True
    ).fit()
    print(m)

    # Estimate printing the covariance matrix.
    m = gretl.ar1(
        "employ ~ const + prdefl + gnp",
        vcv=True
    ).fit()
    print(m)


if __name__ == "__main__":
    run_example()
