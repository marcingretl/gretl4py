import importlib.resources as resources
import gretl

CFfilt = gretl.include("cffilt")
CFfilt.show_functions()

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('denmark.gdt')))
LRM = d1.get_series(series='LRM')

# HP filter
# we call ::hpfilt() with default options
c = d1.hpfilt(y='LRM')
print("\nCycle component from the Hodrick-Prescott filter:")
print(c)

t = d1.hpfilt(y='LRM', one_sided=True, trend=True, hp_lambda=99.78)
print("\nOne-sided trend component from the Hodrick-Prescott filter:")
print(t)

# Butterworth filter
# we call ::bwfilt()
bw = d1.bwfilt(y='LRM', n=4, omega=90)
print("\nResults from the Butterworth filter:")
print(bw)

# Baxter-King bandpass filter with default values
bk = d1.bkfilt(y='LRM')
print("\nResults from the Baxter-King filter:")
print(bk)

# Christiano-Fitzgerald asymmetric filter
cf = gretl.cffilt(x='LRM')
print("\nResults from the Christiano-Fitzgerald asymmetric filter:")
print(cf)

# we generate plot for filtering
try:
    import matplotlib.pyplot as plt

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
