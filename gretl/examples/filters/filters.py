import gretl


def run_example():
    # Load Christiano-Fitzgerald filter package.
    try:
        CFfilt = gretl.include("cffilt")
    except FileNotFoundError:
        gretl.pkg("install", "cffilt")
        CFfilt = gretl.include("cffilt")

    CFfilt.show_functions()

    # Load dataset.
    d = gretl.get_data("denmark.gdt")

    # Hodrick–Prescott filter (default).
    c = d.hpfilt(y="LRM")
    print("\nHP filter cycle component:")
    print(c)

    # Hodrick–Prescott filter (one-sided trend).
    t = d.hpfilt(y="LRM", one_sided=True, trend=True, hp_lambda=99.78)
    print("\nHP filter one-sided trend component:")
    print(t)

    # Butterworth filter.
    bw = d.bwfilt(y="LRM", n=4, omega=90)
    print("\nButterworth filter results:")
    print(bw)

    # Baxter–King filter.
    bk = d.bkfilt(y="LRM")
    print("\nBaxter–King filter results:")
    print(bk)

    # Christiano–Fitzgerald filter.
    cf = gretl.cffilt(x="LRM")
    print("\nChristiano-Fitzgerald filter results:")
    print(cf)

    # Plot results.
    try:
        import matplotlib.pyplot as plt

        lrm = d.get_series(series="LRM")
        x = list(range(1, len(lrm) + 1))

        fig, ax = plt.subplots()
        ax.set_title("LRM series filtering results")

        ax.plot(x, lrm, label="LRM")
        ax.plot(x, t, label="HP trend")
        ax.plot(x, bw, label="Butterworth")
        ax.plot(x, lrm - bk, label="Baxter-King cycle") # this is because of NaN-s
        ax.plot(x, lrm - cf, label="Christiano-Fitzgerald cycle")

        ax.legend()
        plt.show()

    except ModuleNotFoundError:
        print("Please install 'matplotlib' to enable plotting.")


if __name__ == "__main__":
    run_example()
