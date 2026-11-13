import gretl
import matplotlib.pyplot as plt


def show_plot(fig):
    # Display the figure only if one was generated.
    if fig is not None:
        fig.show()


def run_example():
    # load datasets
    d1 = gretl.get_data("data4-1.gdt")
    d2 = gretl.get_data("bjg.gdt")
    d3 = gretl.get_data("greene22_2.gdt")

    # set sample
    d1.sample(1, 10)
    d2.sample(1, d2.n - 4)

    # Disable automatic plot display.
    gretl.set_plot_auto_show(False)

    # ============================================================
    # OLS
    # ============================================================
    print("\n=== OLS ===")

    m1 = gretl.ols(formula="1 0 2 3", data=d1).fit()
    #print(m1)

    # Forecast over a specified observation range.
    b1, f1 = m1.fcast(5, 10, plot="display")

    # Out-of-sample forecast.
    b2, f2 = m1.fcast(out_of_sample=True, plot="display")

    # Forecast over the current sample.
    b3, f3 = m1.fcast(plot="display")

    show_plot(f1)
    show_plot(f2)
    show_plot(f3)

    # ============================================================
    # OLS: named and mixed arguments
    # ============================================================
    print("\n=== OLS: named arguments ===")

    # Forecast range specified using keyword arguments.
    b8, f8 = m1.fcast(
        startobs=5,
        endobs=10,
        plot="none"
    )
    print(b8)

    # Save forecast values to a named series.
    b9, f9 = m1.fcast(
        5, 10,
        vname="fit_named",
        plot="none"
    )
    print(b9)

    # Mix positional and keyword arguments.
    b10, f10 = m1.fcast(
        "fit_mixed",
        startobs=5,
        endobs=10,
        plot="none"
    )
    print(b10)

    # ============================================================
    # OLS: recursive forecasts
    # ============================================================
    print("\n=== OLS: recursive forecasts ===")

    # One-step-ahead recursive forecasts (positional arguments).
    b11, f11 = m1.fcast(
        7, 10, 1,
        recursive=True,
        plot="display"
    )
    print(b11)
    show_plot(f11)

    # Equivalent call using keyword arguments.
    b12, f12 = m1.fcast(
        startobs=7,
        endobs=10,
        steps_ahead=1,
        recursive=True,
        plot="none"
    )
    print(b12)

    # ============================================================
    # ARIMA
    # ============================================================
    # ARIMA
    print("\n=== ARIMA ===")

    m2 = gretl.arima(
        depvar="price",
        order=(1, 1, 1),
        data=d1
    ).fit()

    print(m2)

    b4, f4 = m2.fcast(5, 10, plot="display")
    b5, f5 = m2.fcast(out_of_sample=True, plot="display")

    print(b5)

    show_plot(f4)
    show_plot(f5)

    # ============================================================
    # VAR
    # ============================================================
    # VAR
    print("\n=== VAR ===")

    m4 = gretl.var(1, "g lg", data=d2, quiet=True).fit()
    #print(m4)

    # Select an endogenous variable for forecasting.
    b6, f6 = m4.fcast(
        "g",
        out_of_sample=True,
        plot="display"
    )

    print(b6)
    show_plot(f6)

    # ============================================================
    # VECM
    # ============================================================
    # VECM
    print("\n=== VECM ===")

    m5 = gretl.vecm(1, 1, "g lg", data=d2, quiet=True).fit()

    b7, f7 = m5.fcast("g", plot="display")

    print(b7)
    show_plot(f7)

    # ============================================================
    # Multinomial logit: all-probs
    # ============================================================
    print("\n=== Multinomial logit: all_probs ===")

    # Estimate a multinomial logit model.
    m6 = gretl.logit(
        "Y ~ const + Z1 + Z2",
        multinomial=True,
        data=d3
    ).fit()

    print(m6)

    # Retrieve the predicted probabilities for all outcomes.
    P1, f13 = m6.fcast(all_probs=True)

    print("\nPredicted probabilities:")
    print(P1)
    print("Matrix dimensions:", P1.rows, "x", P1.cols)

    # Save the predicted probabilities to a named gretl matrix.
    P2, f14 = m6.fcast(
        vname="P",
        all_probs=True
    )

    print("\nNamed probability matrix P:")
    print(P2)
    print("Matrix dimensions:", P2.rows, "x", P2.cols)

    # No plot should be generated with all_probs=True.
    assert f13 is None
    assert f14 is None

    # The two calls should return matrices with the same shape.
    assert P1.rows == P2.rows
    assert P1.cols == P2.cols

    # ============================================================
    # Final display
    # ============================================================
    print("\nAll forecast examples completed.")

    plt.show(block=True)


if __name__ == "__main__":
    run_example()
