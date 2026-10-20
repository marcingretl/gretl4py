import gretl


def include_package(name: str):
    """
    Load a gretl package; install it if it is not available locally.
    """
    try:
        pkg = gretl.include(name)
    except FileNotFoundError:
        print(f"Package '{name}' not found. Installing...")
        gretl.pkg("install", name)
        pkg = gretl.include(name)
    return pkg


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
    gretl.set("verbose", "off")

    # ------------------------------------------------------------
    # Load ParMA package (Bayesian model averaging for GLMs)
    # ------------------------------------------------------------
    pkg = include_package("ParMA")
    pkg.show_functions()

    # ------------------------------------------------------------
    # Data
    # ------------------------------------------------------------
    d = gretl.get_data("mroz87.gdt", quiet=True)

    # Covariates for labor force participation model
    X = "KL6 WA WE HA HE HW MTR UN CIT AX"

    # ------------------------------------------------------------
    # Model specification
    # ------------------------------------------------------------
    model_type = "probit"

    # MCMC configuration
    n_iter = 32 * 1024   # posterior draws after burn-in
    burn_in = 2048       # burn-in period
    
    options = gretl.Bundle()
    options["seed"] = 1_234_567
    
    # use MPI if available
    if gretl.about()["sysinfo"]["mpi"]:
        options["mpi"] = gretl.about()["sysinfo"]["ncores"]
    else:
        options["mpi"] = 1

    # ------------------------------------------------------------
    # Bayesian Model Averaging for GLM
    # ------------------------------------------------------------
    start_timer()

    result = gretl.bma_glm(
        "LFP",
        X,
        model_type,
        n_iter,
        burn_in,
        options,
    )

    print_timer()

    return result


if __name__ == "__main__":
    run_example()
