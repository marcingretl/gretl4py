import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("kennan.gdt")

    # Different ways to specify the duration model.
    m1 = gretl.duration("Ti ~ const + Prod").fit()
    m2 = gretl.duration("Ti const Prod").fit()
    m3 = gretl.duration("1 0 2").fit()

    print(m1)
    print(m2)
    print(m3)

    # Estimate exponential duration model.
    m = gretl.duration("Ti ~ const + Prod", exponential=True).fit()
    print(m)

    # Estimate log-logistic duration model.
    m = gretl.duration("Ti ~ const + Prod", loglogistic=True).fit()
    print(m)

    # Estimate log-normal duration model.
    m = gretl.duration("Ti ~ const + Prod", lognormal=True).fit()
    print(m)

    # Report median durations.
    m = gretl.duration("Ti ~ const + Prod", medians=True).fit()
    print(m)

    # Compute robust covariance matrix.
    m = gretl.duration("Ti ~ const + Prod", robust=True).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.duration("Ti ~ const + Prod", vcv=True).fit()
    print(m)

    # Display verbose estimation output.
    m = gretl.duration("Ti ~ const + Prod", verbose=True).fit()
    print(m)


if __name__ == "__main__":
    run_example()
