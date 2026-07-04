import gretl


def run_example():
    # Load dataset.
    d = gretl.get_data("data4-1")

    # Estimate heteroskedasticity-consistent model.
    m = gretl.hsk("price const sqft bedrms").fit()
    print(m)


if __name__ == "__main__":
    run_example()
