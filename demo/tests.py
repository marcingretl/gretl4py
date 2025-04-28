import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))
d2 = gretl.get_data(gretl.gretl_data_path('misc/bjg.gdt'))
d3 = gretl.get_data(gretl.gretl_data_path('misc/grunfeld.gdt'))
d4 = gretl.get_data(gretl.gretl_data_path('ramanathan/data4-10.gdt'))
d5 = gretl.get_data(gretl.gretl_data_path('misc/ooballot.gdt'))
#gretl.show_data_status()

print("\n****************************")
print("adf")
d1.test(type="adf", vars="LRM", order=1, silent=True)
print(d1.adf['summary'])

print("\n****************************")
print("kpss")
d1.test(type="kpss", vars="LRM", order=1, seasonals=True, trend=True)

print("\n****************************")
print("coint")
d1.test(type="coint", y="LRM", vars="LRY IBO", order=4)

print("\n****************************")
print("bds")
d1.test(type="bds", vars="LRY", order=4, quiet=True)
print(d1.bds['summary'])    # issue: empty printout!!!

print("\n****************************")
print("normtest")
d1.test(type="normtest", vars="LRY", swilk=True)

print("\n****************************")
print("runs")
d1.test(type="runs", vars="LRY", difference=True)

print("\n****************************")
print("vartest")
d1.test(type="vartest", vars="LRM IBO")

print("\n****************************")
print("difftest")
d5.new_series("capp = (Vote > 1) ? CPI : NA")
d5.new_series("cdis = (Vote == 0) ? CPI : NA")
d5.test(type="difftest", vars="capp cdis", rank_sum=True)

print("\n****************************")
print("levinlin")
d3.test(type="levinlin", vars="invest", order=[2,2,3,3,4,4,2,3,2,1])

print("\n****************************")
print("meantest")
d3.test(type="meantest", vars="invest firm")
exit()

# OLS
m1 = gretl.ols(formula='1 0 2 3').fit()
print(m1)

print("\n****************************")
print("add")
m1.test(type="add", vars="IDE", silent=True)
print(m1.add['summary'])
print(m1.add['test'])
print(m1.add['pvalue'])

print("\n****************************")
print("omit")
m1.test(type="omit", vars="3")

print("\n****************************")
print("white")
m1.test(type="white")

print("\n****************************")
print("reset")
m1.test(type="reset", cubes_only=True, robust=True)

print("\n****************************")
print("qlrtest")
m1.test(type="qlrtest")

print("\n****************************")
print("cusum")
m1.test(type="cusum", quiet=True)

print("\n****************************")
print("leverage")
m1.test(type="leverage")

print("\n****************************")
print("vif")
m1.test(type="vif")

print("\n****************************")
print("bkw")
m1.test(type="bkw")

print("\n****************************")
print("chow")
m1.test(type="chow", obs="1980:1")


# ARIMA
m2 = gretl.arima(y='lg', order=(2,0,0), data=d2).fit()
print(m2)

print("\n****************************")
print("arch")
m2.test(type="arch", quiet=True)
print(m2.arch['test'])
print(m2.arch['pvalue'])

print("\n****************************")
print("normality")
m2.test(type="normality", quiet=True)

print("\n****************************")
print("autocorr")
m2.test(type="autocorr", order=4)


# PANEL
print("\n****************************")
print("panspec")
m3 = gretl.ols(formula="invest ~ const value kstock", data=d3).fit()
print(m3)
m3.test(type="panspec")

print("\n****************************")
print("panel")
m3.test(type="panel")

# RESTRICT
print("\n****************************")
print("restrict")
m4 = gretl.ols(formula='1 0 2 3 4 5', data=d4).fit()
m4.restrict(impose=['b[4] - b[5] = 0', 'b[3] = 0'])
print(m4.restrictions['pvalue'])

print("\n****************************")
print("coeffsum")
m4.test(type="coeffsum", vars="WHITE ADMEXP")
