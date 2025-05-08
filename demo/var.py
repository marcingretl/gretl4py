import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))

#print(d1.varnames())
m1 = gretl.var(order=2, ylist='LRM LRY', xlist='IBO IDE', trend=True, seasonals=True, impulse_responses=True).fit()
#print(m1.attrs())
print(m1)
print("sigma matrix:")
print(m1.get_accessor("$sigma"))
#m1.test(type="autocorr")
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
