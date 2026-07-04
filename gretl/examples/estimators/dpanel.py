import gretl


def run_example():
    # Load dataset.
    d1 = gretl.get_data("abdata.gdt")

    # Estimate one-step dynamic panel model.
    m1 = gretl.dpanel(
        p=2,
        formula="n w k ; GMM(k,2,3) k GMM(w,2,3)"
    ).fit()
    print(m1)

    # Estimate two-step model using selected lags.
    m2 = gretl.dpanel(
        p=[2, 3],
        formula="n w k ; GMM(k,2,3) k GMM(w,2,3)",
        two_step=True,
    ).fit()
    print(m2)


if __name__ == "__main__":
    run_example()
