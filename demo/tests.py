import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))
d2 = gretl.get_data(gretl.gretl_data_path('misc/bjg.gdt'))
d3 = gretl.get_data(gretl.gretl_data_path('misc/grunfeld.gdt'))
d4 = gretl.get_data(gretl.gretl_data_path('ramanathan/data4-10.gdt'))
#gretl.show_data_status()

print("\n****************************")
print("restrict")
m1 = gretl.ols(formula='1 0 2 3 4 5', data=d4).fit()
m1.restrict(impose=['b[4] = 0', 'b[5] = 0'])
print(m1.restrict['summary'])
exit()

print("\n****************************")
print("adf")
d1.test(type="adf", vars="LRM", order=1)
print(d1.adf['summary'])

print("\n****************************")
print("kpss")
d1.test(type="kpss", vars="LRM", order=1, seasonals=True, trend=True)
print(d1.kpss['summary'])

print("\n****************************")
print("coint")
d1.test(type="coint", y="LRM", vars="LRY IBO", order=4)
print(d1.coint['summary'])

print("\n****************************")
print("bds")
d1.test(type="bds", vars="LRY", order=4)
print(d1.bds['summary'])

print("\n****************************")
print("normtest")
d1.test(type="normtest", vars="LRY", swilk=True)
print(d1.normtest['summary'])

print("\n****************************")
print("runs")
d1.test(type="runs", vars="LRY", difference=True)
print(d1.runs['summary'])

print("\n****************************")
print("vartest")
d1.test(type="vartest", vars="LRM IBO")
print(d1.vartest['summary'])

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
m1.test(type="white")
print(m1.white['summary'])

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
m1.test(type="chow", obs="1980:1")
print(m1.chow['summary'])


# ARIMA
m2 = gretl.arima(y='lg', order=(2,0,0), data=d2).fit()
#print(m2)
#m2.fcast()

print("\n****************************")
print("arch")
m2.test(type="arch")
print(m2.arch['summary'])
print("test:\t " + str(m2.arch['test']))
print("p-value: " + str(m2.arch['pvalue']))

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
