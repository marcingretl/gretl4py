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
    print(f"Elapsed time: {gretl.get_accessor('$stopwatch'):.2f} sec.\n")


def run_example():
    # ------------------------------------------------------------
    # Global settings
    # ------------------------------------------------------------
    gretl.set("verbose", "off")
    gretl.set("seed", 1234567)

    # ------------------------------------------------------------
    # Load package
    # ------------------------------------------------------------
    bt = include_package("BayTool")
    bt.show_functions()

    CASE = "lm"  # lm | lasso | probit | tobit

    # ============================================================
    # Bayesian Linear Model
    # ============================================================
    if CASE == "lm":

        # Koop textbook example: house price regression
        d = gretl.get_data("AnglinGencay.gdt", frompkg="BayTool", quiet=True)

        y = "price"
        X = "const lot_size n_bedroom n_bathroom n_storeys"

        # Independent Normal–Inverse Gamma prior
        prior = gretl.Bundle()
        prior["type"] = "indep"

        # IMPORTANT:
        # gretl.Matrix must be used here (NOT Python list),
        # otherwise it becomes a gretl list, not a matrix.
        prior["beta_mean"] = gretl.Matrix([[0], [10], [5000], [10000], [10000]])            # this is because BayTool reguires column vector
        prior["beta_vcv"] = gretl.Matrix.diag([10000**2, 25, 2500**2, 5000**2, 5000**2])
        prior["precision_mean"] = 4e-08
        prior["precision_df"] = 5

        # Prediction setup
        opt = gretl.Bundle()
        opt["forecast"] = gretl.Matrix([1, 5000, 2, 2, 1])

        start_timer()
        out = gretl.BT_lm(y, X, prior, opt)
        print_timer()

        return out

    # ============================================================
    # Bayesian LASSO
    # ============================================================
    elif CASE == "lasso":

        # Park & Casella (2008)
        d = gretl.get_data("diabetes.gdt", frompkg="BayTool", quiet=True)

        y = "y"
        X = "age sex bmi map tc ldl hdl tch ltg glu"

        # Hyperprior on shrinkage parameter
        prior = gretl.Bundle()
        prior["type"] = "hyper"
        prior["lambda_shape"] = 1
        prior["lambda_rate"] = 1.78

        start_timer()
        out = gretl.BT_lasso(
            y,
            X,
            prior,
            gretl.Bundle(stdize=0),
        )
        print_timer()

        return out

    # ============================================================
    # Bayesian Probit
    # ============================================================
    elif CASE == "probit":

        # Koop–Poirier–Tobias / Fair (1978)
        d = gretl.get_data("Fair_Koop.gdt", frompkg="BayTool", quiet=True)

        y = "affair"
        X = "const male ys_married kids religious ed happy"

        # Weakly informative Normal prior
        prior = gretl.Bundle()
        prior["beta_mean"] = gretl.Matrix(7, 1)
        prior["beta_vcv"] = 100 * gretl.Matrix(7)

        opt = gretl.Bundle()
        opt["iter"] = 2000
        opt["burn"] = 500

        start_timer()
        out = gretl.BT_probit(y, X, prior, opt)
        print_timer()

        return out

    # ============================================================
    # Bayesian Tobit
    # ============================================================
    elif CASE == "tobit":

        # Koop–Poirier–Tobias (Ch. 14, Ex. 14.9)
        d = gretl.get_data("nlsy_Koop_tobit.gdt", frompkg="BayTool", quiet=True)

        y = "weeks"
        X = "const afqt hi kids ed"

        prior = gretl.Bundle()
        prior["beta_mean"] = gretl.Matrix(5, 1)
        prior["beta_vcv"] = 100 * gretl.Matrix(5)
        prior["sigma_a"] = 3
        prior["sigma_b"] = 1 / 40

        opt = gretl.Bundle()
        opt["iter"] = 10000
        opt["rlimit"] = 52

        start_timer()
        out = gretl.BT_tobit(y, X, prior, opt)
        print_timer()

        return out

    # ------------------------------------------------------------
    # Fallback
    # ------------------------------------------------------------
    else:
        raise ValueError(f"Unknown CASE: {CASE}")


if __name__ == "__main__":
    res = run_example()
    d = gretl.get_data("AnglinGencay.gdt", frompkg="BayTool", quiet=True)
    gretl.BT_paramplot(res)
