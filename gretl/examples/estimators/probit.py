import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("greene22_2.gdt")

    # Create binary response variable.
    d1.new_series("rat = Z8 - 1")
    d1.discrete("rat")

    # Different ways to specify probit model.
    m1 = gretl.probit("rat ~ const + Z1 + Z2").fit()
    m2 = gretl.probit("rat const Z1 Z2").fit()
    m3 = gretl.probit("1 0 2 3").fit()

    print(m1)
    print(m2)
    print(m3)

    # Compute robust covariance matrix.
    m = gretl.probit("rat ~ const + Z1 + Z2", robust=True).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.probit("rat ~ const + Z1 + Z2", vcv=True).fit()
    print(m)

    # Display verbose estimation output.
    m = gretl.probit("rat ~ const + Z1 + Z2", verbose=True).fit()
    print(m)

    # Show p-values instead of coefficient estimates.
    m = gretl.probit("rat ~ const + Z1 + Z2", p_values=True).fit()
    print(m)

    # Use alternative pseudo-R² measure (Estrella).
    m = gretl.probit("rat ~ const + Z1 + Z2", estrella=True).fit()
    print(m)

    # Random effects probit model.
    d2 = gretl.get_data("union_wooldridge.gdt")
    m = gretl.probit(
        "union const black married educ dummify(year)",
        data=d2,
        random_effects=True
    ).fit()
    print(m)
    m.plot_new("fitted")

if __name__ == "__main__":
    run_example()
