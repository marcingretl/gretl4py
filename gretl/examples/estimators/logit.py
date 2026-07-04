import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("greene22_2.gdt")

    # Different ways to specify logit model.
    m1 = gretl.logit("Y ~ const + Z1 + Z2").fit()
    m2 = gretl.logit("Y const Z1 Z2").fit()
    m3 = gretl.logit("1 0 2 3").fit()

    print(m1)
    print(m2)
    print(m3)

    # Compute robust covariance matrix.
    m = gretl.logit("Y ~ const + Z1 + Z2", robust=True).fit()
    print(m)

    # Estimate multinomial logit model.
    m = gretl.logit("Y ~ const + Z1 + Z2", multinomial=True).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.logit("Y ~ const + Z1 + Z2", vcv=True).fit()
    print(m)

    # Display verbose estimation output.
    m = gretl.logit("Y ~ const + Z1 + Z2", verbose=True).fit()
    print(m)

    # Show p-values instead of coefficient estimates.
    m = gretl.logit("Y ~ const + Z1 + Z2", p_values=True).fit()
    print(m)

    # Use alternative pseudo-R² measure (Estrella).
    m = gretl.logit("Y ~ const + Z1 + Z2", estrella=True).fit()
    print(m)

    # Odds ratios analysis.
    d2 = gretl.get_data("rac3d.gdt")
    m = gretl.logit("SEX 0 AGE AGESQ INCOME LEVYPLUS", data=d2).fit()
    print(m)
    print(m.oddsratios)


if __name__ == "__main__":
    run_example()
