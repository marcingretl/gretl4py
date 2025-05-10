import gretl
import matplotlib.pyplot as plt

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))

#print(d1.varnames())
m1 = gretl.var(order=2, ylist='LRM LRY', xlist='IBO IDE', trend=True, seasonals=True, impulse_responses=True).fit()
#print(m1.attrs())
print(m1)
print("sigma matrix:")
print(m1.get_accessor("$sigma"))
m1.test(type="autocorr", order=3)
m1.test(type="arch")
print("pvalues:")
print(m1.tests[-1]['pvalue'])
m1.test(type="normality", silent=True, order=3)
print(m1.tests[-1]['summary'])
#m1.fcast()
m1.irf(targ=0, shock=0)
m1.irf(targ='LRM', shock='LRY', alpha=0.1)
m1.irf(targ=1, shock=0, alpha=0.1)

print("\nirf1:")
print(m1.irfs[0])

print("\nirf2:")
print(m1.irfs[1])

print("\nirf3:")
print(m1.irfs[-1])
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
