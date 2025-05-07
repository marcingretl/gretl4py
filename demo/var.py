import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))

#print(d1.varnames())
m1 = gretl.var(order=2, ylist='LRM LRY', xlist='IBO IDE', trend=True, seasonals=True).fit()
#print(m1.attrs())
print(m1)
print(m1.get_accessor("$xtxinv"))
m1.fcast()