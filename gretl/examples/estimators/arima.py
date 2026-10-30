import gretl
import shutil


def run_example():
    # Load dataset.
    d1 = gretl.get_data("bjg.gdt")
    d2 = gretl.get_data("denmark.gdt")

    # Basic ARIMA(1,1,1).
    m = gretl.arima(depvar="lg", order=(1, 1, 1)).fit()
    print(m)

    # Seasonal ARIMA(1,1,1)(1,1,1).
    m = gretl.arima(depvar="lg", order=(1, 1, 1), seasonal=(1, 1, 1)).fit()
    print(m)

    # Verbose output.
    m = gretl.arima(depvar="lg", order=(1, 1, 1), verbose=True).fit()
    print(m)

    # Print covariance matrix.
    m = gretl.arima(depvar="lg", order=(1, 1, 1), vcv=True).fit()
    print(m)

    # Use Hessian-based covariance estimation.
    m = gretl.arima(depvar="lg", order=(1, 1, 1), hessian=True).fit()
    print(m)

    # Use outer product of gradients (OPG).
    m = gretl.arima(depvar="lg", order=(1, 1, 1), opg=True).fit()
    print(m)

    # No constant term.
    m = gretl.arima(depvar="lg", order=(1, 1, 1), nc=True).fit()
    print(m)

    # Conditional estimation.
    m = gretl.arima(depvar="lg", order=(1, 1, 1), conditional=True).fit()
    print(m)

    # X-12-ARIMA (if available).
    d = gretl.about()
    x12_path = d["built_in_strings"]["x12a"]

    if x12_path and shutil.which(x12_path) is not None:
        m = gretl.arima(depvar="lg", order=(1, 1, 1), x_12_arima=True).fit()
        print(m)

    # LBFGS optimizer.
    m = gretl.arima(depvar="lg", order=(1, 1, 1), lbfgs=True).fit()
    print(m)

    # ARIMAX
    m = gretl.arima(depvar="LRY", order=(1, 1, 1), indepvars="IBO", data=d2).fit()
    print(m)
    m.plot_new("fitted")

if __name__ == "__main__":
    run_example()
