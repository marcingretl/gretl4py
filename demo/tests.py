import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))
d2 = gretl.get_data(gretl.gretl_data_path('misc/bjg.gdt'))
d3 = gretl.get_data(gretl.gretl_data_path('misc/grunfeld.gdt'))
#gretl.show_data_status()
#print(d1.get_accessor("$pd"))

# OLS
m1 = gretl.ols(formula='1 0 2 3').fit()
print(m1)

print("\n****************************")
print("add")
m1.test(type="add", vars="IDE")
print(m1.add['summary'])
print(m1.add['test'])
print(m1.add['pvalue'])

print("\n****************************")
print("omit")
m1.test(type="omit", vars="3")
print(m1.omit['summary'])

print("\n****************************")
print("white")
white = m1.test(type="white")
print(white['summary'])

print("\n****************************")
print("reset")
m1.test(type="reset", cubes_only=True, robust=True)
print(m1.reset['summary'])

print("\n****************************")
print("qlrtest")
m1.test(type="qlrtest")
print(m1.qlrtest['summary'])

print("\n****************************")
print("cusum")
m1.test(type="cusum")
print(m1.cusum['summary'])

print("\n****************************")
print("leverage")
m1.test(type="leverage")
print(m1.cusum['summary'])

print("\n****************************")
print("vif")
m1.test(type="vif")
print(m1.vif['summary'])

print("\n****************************")
print("bkw")
m1.test(type="bkw")
print(m1.bkw['summary'])

print("\n****************************")
print("chow")
m1.test(type="chow", vars="1980:1")
print(m1.chow['summary'])


# ARIMA
m2 = gretl.arima(y='lg', order=(2,0,0), data=d2).fit()
print(m2)
#print("pyt: pd")
#print(m2.get_accessor("$pd"))
#print("pyt: vcv")
#print(m2.get_accessor("$vcv"))
#print("pyt: coeff")
#print(m2.get_accessor("$coeff"))
#print(m2.get_accessor("$parnames"))
#print(m2.get_accessor("$xlist"))
#print(m2.get_accessor("$uhat"))
#m2.fcast()

print("\n****************************")
print("arch")
arch_test = m2.test(type="arch")
print(arch_test['summary'])
print("test:\t " + str(arch_test['test']))
print("p-value: " + str(arch_test['pvalue']))

print("\n****************************")
print("normality")
m2.test(type="normality")
print(m2.normality.keys())

print("\n****************************")
print("autocorr")
m2.test(type="autocorr", order=4)
print(m2.autocorr['summary'])



# PANEL
m3 = gretl.ols(formula="invest ~ const value kstock", data=d3).fit()
print(m3)
m3.test(type="panspec")
print(m3.panspec['summary'])

m4 = gretl.panel(formula="invest ~ const value kstock", data=d3).fit()
print(m4)
m4.test(type="panel")
print(m4.panel['summary'])

