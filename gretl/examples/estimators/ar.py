import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("longley.csv")

    # Estimate AR(2) model.
    m = gretl.ar("1 2 ; employ const prdefl gnp armfrc").fit()
    print(m)

    # Estimate the same model printing the covariance matrix.
    m = gretl.ar(
        "1 2 ; employ const prdefl gnp armfrc",
        vcv=True
    ).fit()
    print(m)

    # Compute correlogram diagnostic.
    m.test(type="corrgm")


if __name__ == "__main__":
    run_example()
