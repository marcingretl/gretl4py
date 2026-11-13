import gretl

# Disable automatic display.
gretl.set_plot_auto_show(False)

def run_example():
    # Load dataset.
    d1 = gretl.get_data("b-g.gdt")

    spec = "1 1 ; Y"

    # Estimate GARCH(1,1) model.
    m = gretl.garch(spec).fit()
    print(m)

    # Compute robust covariance matrix.
    m = gretl.garch(spec, robust=True).fit()
    print(m)

    # Display verbose estimation output.
    m = gretl.garch(spec, verbose=True).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.garch(spec, vcv=True).fit()
    print(m)

    # Estimate model without a constant term.
    m = gretl.garch(spec, nc=True).fit()
    print(m)

    # Standardized residuals.
    m = gretl.garch(spec, stdresid=True).fit()
    print(m)

    # Use the Fiorentini–Calzolari–Panattoni (FCP) algorithm.
    m = gretl.garch(spec, fcp=True).fit()
    print(m)

    # Initialize variance parameters from an ARMA model.
    m = gretl.garch(spec, arma_init=True).fit()
    print(m)
    fig = m.plot_new("residuals")

    # uncomment to plot figure
    #import matplotlib.pyplot as plt
    #plt.show(block=True)


if __name__ == "__main__":
    run_example()
