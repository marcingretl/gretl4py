import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("longley.csv")

    # Estimate multiple precision OLS model.
    m = gretl.mpols("employ ~ const + prdefl + gnp").fit()
    print(m)

    # Print covariance matrix.
    m = gretl.mpols(
        "employ ~ const + prdefl + gnp",
        vcv=True
    ).fit()
    print(m)

    # Suppress auxiliary statistics in output.
    m = gretl.mpols(
        "employ ~ const + prdefl + gnp",
        simple_print=True
    ).fit()
    print(m)


if __name__ == "__main__":
    run_example()
