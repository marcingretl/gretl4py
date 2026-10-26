import gretl


def run_example():
    # ------------------------------------------------------------
    # Global settings
    # ------------------------------------------------------------
    # Turn off verbose output for cleaner example output
    gretl.set("verbose", "off")

    # ------------------------------------------------------------
    # Load GIG package (external GARCH toolkit)
    # ------------------------------------------------------------
    gretl.include("gig")

    # ------------------------------------------------------------
    # Data preparation
    # ------------------------------------------------------------
    # Dow Jones closing prices dataset
    djclose = gretl.get_data("djclose.gdt")

    # Construct log returns: 100 * log-difference
    djclose.new_series("rr = 100*ldiff(djclose)")

    # ------------------------------------------------------------
    # GIG workflow: structured estimation API
    # ------------------------------------------------------------
    # Setup GARCH model object (no mean regressors)
    gig_model = gretl.gig_setup("rr")

    # Estimate model
    gretl.gig_estimate(gig_model)
    gretl.gig_plot(gig_model)

    # ------------------------------------------------------------
    # Native gretl GARCH comparison
    # ------------------------------------------------------------
    # Standard GARCH(1,1) with Normal errors
    native_model = gretl.garch(
        "1 1 ; rr",
        nc=True,      # no constant in mean equation
        robust=True,  # robust covariance estimation
    ).fit()

    print(native_model)

    return gig_model, native_model


if __name__ == "__main__":
    run_example()
