import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("penngrow.gdt")

    # Estimate two-stage least squares model.
    m = gretl.tsls("Y Y(-1) X ; 0 X Y(-2)").fit()
    print(m)

    # Suppress diagnostic tests.
    m = gretl.tsls("Y Y(-1) X ; 0 X Y(-2)", no_tests=True).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.tsls("Y Y(-1) X ; 0 X Y(-2)", vcv=True).fit()
    print(m)

    # No degrees-of-freedom correction.
    m = gretl.tsls("Y Y(-1) X ; 0 X Y(-2)", no_df_corr=True).fit()
    print(m)

    # Compute robust covariance matrix.
    m = gretl.tsls("Y Y(-1) X ; 0 X Y(-2)", robust=True).fit()
    print(m)

    # Limited Information Maximum Likelihood (LIML).
    # m = gretl.tsls("Y Y(-1) X ; 0 X Y(-2)", liml=True).fit()
    # print(m)

    # Generalized Method of Moments (GMM).
    # m = gretl.tsls("Y Y(-1) X ; 0 X Y(-2)", gmm=True).fit()
    # print(m)


if __name__ == "__main__":
    run_example()
