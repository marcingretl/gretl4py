import gretl


def run_example():
    # Load dataset.
    d = gretl.get_data("gdp_midas.gdt")

    # Define MIDAS weighting parameters.
    gretl.genr("theta = {1,5}")

    # Build MIDAS regressors.
    d.new_list(name="dX", value="ld_payems*")
    d.new_list("dXL = hflags(3, 11, dX)")

    # Estimate MIDAS regression model.
    m = gretl.midasreg(
        formula="ld_qgdp 0 ld_qgdp(-1) ; mdsl(dXL, 2, theta)"
    ).fit()

    print(m)


if __name__ == "__main__":
    run_example()
