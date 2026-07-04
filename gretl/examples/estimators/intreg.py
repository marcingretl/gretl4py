import gretl


def run_example():
    # Load dataset.
    d = gretl.get_data("wtp")

    # Construct interval bounds via sample restrictions.

    d.sample("depvar == 1", restrict=True)
    d.new_series("hi = bidl")

    d.sample("depvar == 2", restrict=True, replace=True)
    d.new_series("lo = bidl")
    d.new_series("hi = bid1")

    d.sample("depvar == 3", restrict=True, replace=True)
    d.new_series("lo = bid1")
    d.new_series("hi = bidh")

    d.sample("depvar == 4", restrict=True, replace=True)
    d.new_series(name="lo", value="bidh")

    # Restore full sample.
    d.sample(full=True)

    # Estimate interval regression model.
    m = gretl.intreg("lo hi const age female income").fit()
    print(m)


if __name__ == "__main__":
    run_example()
