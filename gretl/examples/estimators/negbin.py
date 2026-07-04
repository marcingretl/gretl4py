import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("rac3d.gdt")

    # Different ways to specify negative binomial model.
    m1 = gretl.negbin("DVISITS ~ const + SEX + AGE").fit()
    m2 = gretl.negbin("DVISITS const SEX AGE").fit()
    m3 = gretl.negbin("13 0 1 2").fit()

    print(m1)
    print(m2)
    print(m3)

    # Use NegBin type 1 specification.
    m = gretl.negbin(
        "DVISITS ~ const + SEX + AGE",
        model1=True
    ).fit()
    print(m)

    # Compute robust covariance matrix.
    m = gretl.negbin(
        "DVISITS ~ const + SEX + AGE",
        robust=True
    ).fit()
    print(m)

    # Use the outer product of gradients (OPG).
    m = gretl.negbin(
        "DVISITS ~ const + SEX + AGE",
        opg=True
    ).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.negbin(
        "DVISITS ~ const + SEX + AGE",
        vcv=True
    ).fit()
    print(m)

    # Display verbose estimation output.
    m = gretl.negbin(
        "DVISITS ~ const + SEX + AGE",
        verbose=True
    ).fit()
    print(m)


if __name__ == "__main__":
    run_example()
