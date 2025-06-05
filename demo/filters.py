import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('denmark.gdt')))
LRM = d1.get_series(series='LRM')

# HP filter
# we call ::hpfilt() with default options
c = d1.hpfilt(series='LRM')
print("\nCycle component from the Hodrick-Prescott filter:")
print(c)

t = d1.hpfilt(series='LRM', one_sided=True, trend=True, hp_lambda=99.78)
print("\nOne-sided trend component from the Hodrick-Prescott filter:")
print(t)

# Butterworth filter
# we call ::bwfilt()
bw = d1.bwfilt(series='LRM', order=4, omega=90)
print("\nResults from the Butterworth filter:")
print(bw)

# Baxter-King bandpass filter with default values
bk = d1.bkfilt(series='LRM')
print("\nResults from the Baxter-King filter:")
print(bk)

# we generate plot for filtering
try:
    import matplotlib.pyplot as plt

    x = list(range(1, len(LRM) + 1))
    bk_trend = LRM - bk

    fig, ax = plt.subplots()
    plt.title("LRM series filtering results")
    ax.plot(x, LRM, label='LRM')
    ax.plot(x, t, label='HP')
    ax.plot(x, bw, label='Butterworth')
    ax.plot(x, bk_trend, label='Baxter-King')
    plt.legend()
    plt.show()
except ModuleNotFoundError:
    print("please install 'matplotlib' module")

