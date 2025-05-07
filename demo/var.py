import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))

#print(d1.varnames())
m1 = gretl.var(order=2, ylist='LRM LRY').fit()
print(m1)
m1.fcast()
print(m1.get_accessor("$lnl"))
#print(m1.attrs())
