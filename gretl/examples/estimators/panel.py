import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("penngrow.gdt")

    # Different ways to specify panel model.
    m1 = gretl.panel("Y ~ const + Y(-1) + X").fit()
    m2 = gretl.panel("Y const Y(-1) X").fit()

    print(m1)
    print(m2)

    # Print covariance matrix.
    m = gretl.panel("Y ~ const + Y(-1) + X", vcv=True).fit()
    print(m)

    # Fixed effects model.
    m = gretl.panel("Y ~ const + Y(-1) + X", fixed_effects=True).fit()
    print(m)

    # Random effects (GLS) model.
    m = gretl.panel("Y ~ const + Y(-1) + X", random_effects=True).fit()
    print(m)

    # Nerlove transformation.
    m = gretl.panel("Y ~ const + Y(-1) + X", random_effects=True, nerlove=True).fit()
    print(m)

    # Pooled OLS model.
    m = gretl.panel("Y ~ const + Y(-1) + X", pooled=True).fit()
    print(m)

    # Between-groups model.
    m = gretl.panel("Y ~ const + Y(-1) + X", between=True).fit()
    print(m)

    # Compute robust covariance matrix.
    m = gretl.panel("Y ~ const + Y(-1) + X", robust=True).fit()
    print(m)

    # Add time dummies.
    m = gretl.panel("Y ~ const + Y(-1) + X", time_dummies=True).fit()
    print(m)

    # Apply unit weights.
    m = gretl.panel("Y ~ const + Y(-1) + X", unit_weights=True).fit()
    print(m)

    # Iterative estimation.
    m = gretl.panel("Y ~ const + Y(-1) + X", iterate=True).fit()
    print(m)

    # Hausman test via matrix difference.
    m = gretl.panel("Y ~ const + Y(-1) + X", matrix_diff=True).fit()
    print(m)

    # Display verbose estimation output.
    m = gretl.panel("Y ~ const + Y(-1) + X", verbose=True).fit()
    print(m)


if __name__ == "__main__":
    run_example()
