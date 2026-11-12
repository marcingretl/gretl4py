import gretl
import numpy as np
import matplotlib.pyplot as plt

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
    m1.plot.residuals(mode="multiple")

    # Jalpha matrix.
    print("Jalpha:")
    print(m1.get_accessor("$jalpha"))

    # Model-based tests.
    m1_autocorr = m1.autocorr(order=3)
    print("Autocorrelation test statistics:")
    print(m1_autocorr["test"])

    m1_arch = m1.arch()
    print("ARCH test p-values:")
    print(m1_arch["pvalue"])

    m1_normality = m1.normality(silent=True)
    print("Normality test summary:")
    print(m1_normality["summary"])

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

    # IRF plotting
    # Extract the IRF and its lower and upper deviations.
    a = irf2.numpy_view
    y, l, u = a[:, 0], a[:, 1], a[:, 2]

    shock_name = "LRY"
    target_name = "LRM"

    # Construct the confidence band: center and half-width.
    bandmat = gretl.Matrix(np.column_stack((
        y + (u - l) / 2,
        (l + u) / 2
    )))

    band = gretl.Bundle()
    band["bandmat"] = bandmat
    band["factor"] = 1.0
    band["style"] = "fill"

    # Generate the IRF plot using libgretl and gptranslate.
    fig = irf2.plot(
        "1",
        time_series=True,
        with_lp=True,
        band=band
    )

    # Adjust the figure layout and title.
    if fig is not None:
        fig.axes[0].set_title(
            f"{target_name}: responses to a one-standard-error shock in {shock_name}"
        )
        fig.subplots_adjust(top=0.95)
        fig.canvas.draw_idle()    
    
    # # Pure Matplotlib implementation
    # x = list(range(1, len(y) + 1))
    # shock_name = "LRY"
    # target_name = "LRM"
    # fig, ax = plt.subplots()
    # ax.set_title(
    #     f"{target_name}: responses to a one-standard-error shock in {shock_name}"
    # )
    # ax.plot(x, y, "-")
    # ax.fill_between(x, y - l, y + u, alpha=0.2)
    # ax.plot(x, y, "o")


if __name__ == "__main__":
    run_example()
    plt.show()
