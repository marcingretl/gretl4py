import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("mroz87.gdt")

    formula = "WW const AX WE CIT ; LFP const WA FAMINC WE"

    # Estimate Heckman selection model.
    m = gretl.heckit(formula).fit()
    print(m)

    # Perform two-step estimation.
    m = gretl.heckit(formula, two_step=True).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.heckit(formula, vcv=True).fit()
    print(m)

    # Use the outer product of gradients (OPG).
    m = gretl.heckit(formula, opg=True).fit()
    print(m)

    # Compute robust covariance matrix.
    m = gretl.heckit(formula, robust=True).fit()
    print(m)

    # Display verbose estimation output.
    m = gretl.heckit(formula, verbose=True).fit()
    print(m)


if __name__ == "__main__":
    run_example()

