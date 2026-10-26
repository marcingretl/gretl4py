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
    # Load TVC package (time-varying coefficient models)
    # ------------------------------------------------------------
    include_package("TVCplots")
    tvc = include_package("TVC")    
    tvc.show_functions()

    # ------------------------------------------------------------
    # Data
    # ------------------------------------------------------------
    d = gretl.get_data("OkunGer.gdt", frompkg="TVC", quiet=True)

    # Regressors: GDP growth and intercept
    L = ['gGDP', 'intercept']

    # ------------------------------------------------------------
    # Time-varying coefficient estimation
    # ------------------------------------------------------------
    # Basic TVC estimation (no additional scaling ratios)
    P = gretl.TVCestimate(
        "dU",
        L,
        plt=True,
    )

    # Alternative specification with scaling ratios:
    #
    # P = gretl.TVCestimate(
    #     "dU",
    #     L,
    #     ratios=gretl.Matrix([1, 0.8]),
    #     plt=True,
    # )

    # ------------------------------------------------------------
    # Output control
    # ------------------------------------------------------------
    gretl.set("echo", "on")

    return P


if __name__ == "__main__":
    res = run_example()
    print(res)
