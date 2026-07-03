import gretl


def run_example():
    # Load the example dataset.
    d1 = gretl.get_data("longley.csv")

    # Select observations 5 through 10 and print the subsample.
    d1.sample(5, 10)
    d1.print()

    # Restore the full sample.
    d1.sample(full=True)

    # Restrict the sample to observations with armfrc >= 2000.
    d1.sample("armfrc>=2000", restrict=True, replace=True)


if __name__ == "__main__":
    run_example()
