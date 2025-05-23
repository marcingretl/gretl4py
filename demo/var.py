import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('denmark.gdt')))
d1.sample(t1=1, t2=51)

#print(d1.varnames())
m1 = gretl.var(order=2, ylist='LRM LRY', xlist='IBO IDE', trend=True, seasonals=True, impulse_responses=True).fit()
#print(m1.attrs())
print(m1)
print("sigma matrix:")
print(m1.get_accessor("$sigma"))

m1.test(type="autocorr", order=3)
print("test statistics:")
print(m1.tests[-1]['test'])

m1.test(type="arch")
print("pvalues:")
print(m1.tests[-1]['pvalue'])

m1.test(type="normality", silent=True)
print(m1.tests[-1]['summary'])

#FORECASTS
print("\n****************************")
print("forecasts")
m1.fcast()

#IRFS
print("\n****************************")
print("IRFs")
m1.irf(targ=0, shock=0)
m1.irf(targ='LRM', shock='LRY', alpha=0.1)
m1.irf(targ=1, shock=0, alpha=0.1)

print("\nirf1:")
print(m1.irfs[0])

print("\nirf2:")
print(m1.irfs[1])

print("\nirf3:")
print(m1.irfs[-1])
try:
    import matplotlib.pyplot as plt

    y = m1.irfs[-1]['results'][:, 0]
    l = m1.irfs[-1]['results'][:, 1]
    u = m1.irfs[-1]['results'][:, 2]
    x = list(range(1, len(y) + 1))
    shock_name = m1.irfs[-1]['shock_name']
    targ_name = m1.irfs[-1]['targ_name']

    fig, ax = plt.subplots()
    plt.title(targ_name + ": responses to a one-standard error shock in " + shock_name)
    ax.plot(x, y, '-')
    ax.fill_between(x, y - l, y + u, alpha=0.2)
    ax.plot(x, y, 'o', color='tab:brown')
    plt.show()
except ModuleNotFoundError:
    print("please install 'matplotlib' module")
