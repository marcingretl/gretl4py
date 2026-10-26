import gretl


def run_example():
    # Load the BACE package, installing it if necessary.
    try:
        package = gretl.include("BACE")
    except FileNotFoundError:
        print("Installing the 'BACE' package...")
        gretl.pkg("install", "BACE")
        package = gretl.include("BACE")

    # Display the functions exported by the package.
    package.show_functions()

    # Load the example dataset bundled with the package.
    d = gretl.get_data("nels.gdt", frompkg="BACE", quiet=True)

    # Set the random seed for reproducibility.
    gretl.set("seed", 1000000)

    # Measure execution time.
    gretl.set("stopwatch")

    # Run Bayesian Averaging of Classical Estimates (BACE).
    result = gretl.BACE(
        "psechoice",
        "2..14",
        gretl.Bundle(model_type="probit"),
    )

    # Equivalent keyword-based call:
    #
    # result = gretl.BACE(
    #     Y="psechoice",
    #     X_list="2..14",
    #     Options=gretl.Bundle(model_type="probit"),
    # )

    elapsed = gretl.get_accessor("$stopwatch")
    print(f"\nTotal execution time: {elapsed:.2f} seconds")
    
    gretl.BACE_Plot(result)

    return


if __name__ == "__main__":
    run_example()
    
