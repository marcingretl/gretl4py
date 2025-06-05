import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('denmark.gdt')))
LRM = d1.get_series(series='LRM')

# we call ::hpfilt() with default options
c = d1.hpfilt(series='LRM')
print("\nCycle component from the Hodrick-Prescott filter:")
print(c)

t = d1.hpfilt(series='LRM', one_sided=True, trend=True, hp_lambda=99.78)
print("\nOne-sided trend component from the Hodrick-Prescott filter:")
print(t)

# we generate plot
try:
    import matplotlib.pyplot as plt

    x = list(range(1, len(LRM) + 1))

    fig, ax = plt.subplots()
    plt.title("One-sided trend component from the Hodrick-Prescott filter")
    ax.plot(x, LRM, label='LRM')
    ax.plot(x, t, label='HP')
    plt.legend()
    plt.show()
except ModuleNotFoundError:
    print("please install 'matplotlib' module")

