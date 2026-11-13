import gretl


def run_example():
    # ------------------------------------------------------------
    # Global settings
    # ------------------------------------------------------------
    gretl.set("verbose", "off")

    # ------------------------------------------------------------
    # Load SVAR package
    # ------------------------------------------------------------
    sv = gretl.include("SVAR")
    sv.show_functions()

    # ------------------------------------------------------------
    # Data preparation
    # ------------------------------------------------------------
    d = gretl.get_data("sw_ch14.gdt")

    # Inflation transformation: annualized log-difference
    d.new_series(name="infl", value="400*ldiff(PUNEW)")

    # Rename unemployment series for clarity
    d.rename("LHUR", "unemp")

    # Endogenous and exogenous variables
    X = ['unemp', 'infl']
    Z = ['const']

    # ------------------------------------------------------------
    # SVAR model setup
    # ------------------------------------------------------------
    # Structural VAR with identification type "C", lag order 3
    model = gretl.SVAR_setup("C", X, Z, 3)

    # Forecast horizon for impulse responses / FEVD
    model["horizon"] = 36

    # ------------------------------------------------------------
    # Identification restrictions
    # ------------------------------------------------------------
    gretl.SVAR_restrict(model, "C", 1, 2)

    # ------------------------------------------------------------
    # Estimation
    # ------------------------------------------------------------
    gretl.set("stopwatch")
    gretl.SVAR_estimate(model)

    print(f"Time (SVAR estimation) = {gretl.get_accessor('$stopwatch'):.2g}")
    
    # ------------------------------------------------------------
    # HD (Plot Historical Decomposition)
    # ------------------------------------------------------------
    gretl.HDplot(model, 2)

    # ------------------------------------------------------------
    # FEVD (Forecast Error Variance Decomposition)
    # ------------------------------------------------------------
    fevd = gretl.FEVD(model)
    print(fevd)
    gretl.IRFplot(model)

    # ------------------------------------------------------------
    # Bootstrap inference
    # ------------------------------------------------------------
    gretl.set("stopwatch")

    # failed = gretl.SVAR_boot(model, 1024, 0.90)
    # 
    # print(
    #     f"Bootstrap failures = {int(failed)}, "
    #     f"time = {gretl.get_accessor('$stopwatch'):.2g}"
    # )

    return model, fevd


if __name__ == "__main__":
    run_example()
    import matplotlib.pyplot as plt
    plt.show()
