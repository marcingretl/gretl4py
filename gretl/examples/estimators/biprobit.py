import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("greene25_1.gdt")

    # Estimate bivariate probit model.
    m = gretl.biprobit(
        "anydrg cardhldr const age avgexp ; "
        "const age income ownrent selfempl"
    ).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.biprobit(
        "anydrg cardhldr const age avgexp ; "
        "const age income ownrent selfempl",
        vcv=True,
    ).fit()
    print(m)

    # Compute robust covariance matrix.
    m = gretl.biprobit(
        "anydrg cardhldr const age avgexp ; "
        "const age income ownrent selfempl",
        robust=True,
    ).fit()
    print(m)

    # Use the outer product of gradients (OPG).
    m = gretl.biprobit(
        "anydrg cardhldr const age avgexp ; "
        "const age income ownrent selfempl",
        opg=True,
    ).fit()
    print(m)

    # Save fitted latent indices.
    m = gretl.biprobit(
        "anydrg cardhldr const age avgexp ; "
        "const age income ownrent selfempl",
        save_xbeta=True,
    ).fit()
    print(m)
    print(m.bp_yhat)

    # Display verbose estimation output.
    m = gretl.biprobit(
        "anydrg cardhldr const age avgexp ; "
        "const age income ownrent selfempl",
        verbose=True,
    ).fit()
    print(m)


if __name__ == "__main__":
    run_example()
