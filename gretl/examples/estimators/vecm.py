import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("denmark.gdt")
    d1.sample(1, 51)

    # Estimate VECM model.
    m1 = gretl.vecm(
        order=2,
        rank=1,
        ylist="LRM LRY",
        xlist="IBO IDE",
        rc=True,
        seasonals=True,
    ).fit()

    print(m1)

    # Jalpha matrix.
    print("Jalpha:")
    print(m1.get_accessor("$jalpha"))

    # Model-based tests.
    m1.test(type="autocorr", order=3)
    print("Autocorrelation test statistics:")
    print(m1.tests[-1]["test"])

    m1.test(type="arch")
    print("ARCH test p-values:")
    print(m1.tests[-1]["pvalue"])

    m1.test(type="normality", silent=True)
    print("Normality test summary:")
    print(m1.tests[-1]["summary"])

    # Save residuals.
    uhat_name = m1.save_uhat(eqn=2)
    print(f"Residuals added as: {uhat_name}")

    # Save error-correction term.
    ec_name = m1.save_ec_term(nr=1)
    print(f"EC term added as: {ec_name}")

    # Restrictions.
    print("\nRestrictions")
    m1.restrict(impose=["b[2] = 0"], verbose=True)
    print(m1.restrictions[-1]["pvalue"])

    # Forecasts.
    print("\nForecasts")
    m1.fcast(out_of_sample=True)

    # Impulse response functions (IRFs).
    irf1 = m1.irf(target=0, shock=0)
    irf2 = m1.irf(target="LRM", shock="LRY", alpha=0.1)
    irf3 = m1.irf(target=1, shock=0, alpha=0.1)

    print("\nIRF outputs:")
    print(irf1)
    print(irf2)
    print(irf3)

    # IRF plotting (optional dependency).
    try:
        import matplotlib.pyplot as plt

        y = irf2.numpy_view[:, 0]
        l = irf2.numpy_view[:, 1]
        u = irf2.numpy_view[:, 2]
        x = list(range(1, len(y) + 1))

        shock_name = "LRY"
        target_name = "LRM"

        fig, ax = plt.subplots()
        ax.set_title(
            f"{target_name}: responses to a one-standard-error shock in {shock_name}"
        )

        ax.plot(x, y, "-")
        ax.fill_between(x, y - l, y + u, alpha=0.2)
        ax.plot(x, y, "o")

        plt.show()

    except ModuleNotFoundError:
        print("Please install 'matplotlib' to enable plotting.")


if __name__ == "__main__":
    run_example()
