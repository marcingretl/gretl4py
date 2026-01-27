import gretl

def run_example():
    # we load cffilt package with Christiano-Fitzgerald asymmetric filter implementation
    try:
        CFfilt = gretl.include('cffilt')
    except FileNotFoundError:
        gretl.pkg('install', 'cffilt')
        CFfilt = gretl.include('cffilt')
    CFfilt.show_functions()

    # we load dataset
    d = gretl.get_data('denmark.gdt')

    # Hodrick-Prescot: ::hpfilt() with default options
    c = d.hpfilt(y='LRM')
    print("\nCycle component from the Hodrick-Prescott filter:")
    print(c)

    t = d.hpfilt(y='LRM', one_sided=True, trend=True, hp_lambda=99.78)
    print("\nOne-sided trend component from the Hodrick-Prescott filter:")
    print(t)

    # Butterworth: ::bwfilt() with given options
    bw = d.bwfilt(y='LRM', n=4, omega=90)
    print("\nResults from the Butterworth filter:")
    print(bw)

    # Baxter-King: ::bkfilt() with default options
    bk = d.bkfilt(y='LRM')
    print("\nResults from the Baxter-King filter:")
    print(bk)

    # Christiano-Fitzgerald: cffilt() with default options
    cf = gretl.cffilt(x='LRM')
    print("\nResults from the Christiano-Fitzgerald asymmetric filter:")
    print(cf)

    # we generate plot
    try:
        import matplotlib.pyplot as plt

        LRM = d.get_series(series='LRM')
        x = list(range(1, len(LRM) + 1))
        fig, ax = plt.subplots()
        plt.title("LRM series filtering results")
        ax.plot(x, LRM, label='LRM')
        ax.plot(x, t, label='hpfilt')
        ax.plot(x, bw, label='bwfilt')
        ax.plot(x, LRM - bk, label='bkfilt')
        ax.plot(x, LRM - cf, label='cffilt')
        plt.legend()
        plt.show()
    except ModuleNotFoundError:
        print("please install 'matplotlib' module")

if __name__ == "__main__":
    run_example()
