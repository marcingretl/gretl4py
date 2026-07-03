import gretl


def run_example():
    # ------------------------------------------------------------
    # Environment setup
    # ------------------------------------------------------------
    # Ensure consistent numeric formatting (decimal point)
    gretl.set("force_decpoint", "on")

    # ------------------------------------------------------------
    # Load data
    # ------------------------------------------------------------
    d = gretl.get_data("longley.csv")

    # ------------------------------------------------------------
    # Load criteria package
    # ------------------------------------------------------------
    crit = gretl.include("criteria")

    # Show available functions in the package
    crit.show_functions()

    # ------------------------------------------------------------
    # OLS estimation
    # ------------------------------------------------------------
    # Longley dataset regression (classic benchmark example)
    model = gretl.ols(formula="1 0 2 3 4 5 6 7", simple=True).fit()

    print(model)

    # ------------------------------------------------------------
    # Export model statistics to gretl environment
    # ------------------------------------------------------------
    # These are later used by the criteria package
    gretl.genr(name="ess", value=model.ess)
    gretl.genr(name="k", value=model.ncoeff)
    gretl.genr(name="n", value=model.T)

    # ------------------------------------------------------------
    # Information criteria: different calling styles
    # ------------------------------------------------------------

    # 1. Positional arguments only
    res = gretl.criteria("ess", model.T, "k")
    print(res)

    # 2. Keyword arguments (order-independent)
    res2 = gretl.criteria(
        n=model.T,
        k="k",
        ess="ess",
    )
    print(res2)

    # 3. Mixed positional + keyword arguments
    res3 = gretl.criteria(
        "ess",
        k="k",
        n=model.T,
    )
    print(res3)

    return model, res, res2, res3


if __name__ == "__main__":
    run_example()
