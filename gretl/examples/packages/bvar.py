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


def run_example():
    # ------------------------------------------------------------
    # Global settings
    # ------------------------------------------------------------
    gretl.set("verbose", "off")
    gretl.set("seed", 1234)

    # ------------------------------------------------------------
    # Load BVAR package
    # ------------------------------------------------------------
    bv = include_package("BVAR")
    bv.show_functions()

    # ------------------------------------------------------------
    # Data
    # ------------------------------------------------------------
    d = gretl.get_data("koopkoro.gdt", frompkg="BVAR")

    # Three-variable system (as space-separated string)
    y = "1 2 3"

    # ------------------------------------------------------------
    # Step 1: Model setup
    # ------------------------------------------------------------
    # Conjugate Bayesian VAR with lag order 4
    model = gretl.BVAR_setup(y, 4, "conj")

    # ------------------------------------------------------------
    # Step 2: Prior specification
    # ------------------------------------------------------------

    # Sigma prior: AR structure
    gretl.BVAR_sigma_prior(model, "AR")

    # Coefficient prior: Minnesota-type prior with zero mean
    gretl.BVAR_alpha_prior(
        model,
        "Minn",
        gretl.Bundle(p_mean=0),
    )

    # ------------------------------------------------------------
    # Step 3: Posterior simulation
    # ------------------------------------------------------------
    res = gretl.BVAR_posterior(
        model,
        "all",
        gretl.Bundle(iter=10000),
    )

    # Display results as tree structure
    res.print(tree=True)

    return res


if __name__ == "__main__":
    run_example()
