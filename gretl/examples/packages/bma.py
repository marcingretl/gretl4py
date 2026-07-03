import gretl


def include_package(name: str):
    """
    Load a gretl package; install it if it is not available locally.
    """
    try:
        return gretl.include(name)
    except FileNotFoundError:
        print(f"Package '{name}' not found. Installing...")
        gretl.pkg("install", name)
        return gretl.include(name)


def start_timer():
    """Start gretl internal stopwatch."""
    gretl.set("stopwatch")


def print_timer():
    """Print elapsed time from gretl stopwatch."""
    print(f"Total execution time: {gretl.get_accessor('$stopwatch'):.2f} sec.\n")


def run_example():
    # ------------------------------------------------------------
    # Global settings
    # ------------------------------------------------------------
    gretl.set("seed", 1_000_000)

    # ------------------------------------------------------------
    # Load BMA package
    # ------------------------------------------------------------
    pkg = include_package("BMA")

    # Optional: inspect available functions
    pkg.show_functions()

    # ------------------------------------------------------------
    # Dataset and model specification
    # ------------------------------------------------------------

    # Fernández, Ley & Steel (2001)
    # Model uncertainty in cross-country growth regressions
    d = gretl.get_data("FLS_41t72", frompkg="BMA")

    # Candidate regressors (2..42 = column range in dataset)
    factors = "2..42"

    # ------------------------------------------------------------
    # MC3 configuration
    # ------------------------------------------------------------
    opts = gretl.Bundle()

    # NOTE: MC3 settings can be very computationally intensive
    opts["Nrep"] = 3000000     # number of MC3 iterations
    opts["burn"] = 33          # burn-in period
    opts["Nrank"] = 20         # top models to store
    opts["do_joint"] = "myq"   # joint move strategy

    # ------------------------------------------------------------
    # Bayesian Model Averaging estimation
    # ------------------------------------------------------------
    start_timer()

    res = gretl.BMA(
        "GDP_growth",
        factors,
        opts,
    )

    print_timer()

    return res


if __name__ == "__main__":
    run_example()
