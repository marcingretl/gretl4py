import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

d1 = gretl.get_data(str(data_dir.joinpath('denmark.gdt')))
d2 = gretl.get_data(str(data_dir.joinpath('bjg.gdt')))
d3 = gretl.get_data(str(data_dir.joinpath('grunfeld.gdt')))
d4 = gretl.get_data(str(data_dir.joinpath('data4-10.gdt')))
d5 = gretl.get_data(str(data_dir.joinpath('ooballot.gdt')))

#gretl.show_data_status()

print("\n****************************")
print("adf")
d1.test(type="adf", vars="LRM", order=1, silent=True)
print(d1.tests[-1]['summary'])

print("\n****************************")
print("kpss")
d1.test(type="kpss", vars="LRM", order=1, seasonals=True, trend=True)

print("\n****************************")
print("coint")
d1.test(type="coint", y="LRM", vars="LRY IBO", order=4)

print("\n****************************")
print("johansen")
d1.test(type="johansen", vars="LRM LRY IBO IDE", order=4, rc=True)

print("\n****************************")
print("bds")
d1.test(type="bds", vars="LRY", order=4, quiet=True)
print(d1.tests[-1]['summary'])    # issue: empty printout!!!

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
print(d3.tests[-1]['pvalue'])

print("\n****************************")
print("corrgm")
d1.test(type="corrgm", y="LRM", order=8)
print(d1.tests[-1]['pvalue'])

# OLS
m1 = gretl.ols(formula='1 0 2 3').fit()
print(m1)

print("\n****************************")
m1.test(type="add", vars="IDE", silent=True)
print(m1.tests[-1]['type'])
print(m1.tests[-1]['options'])
print(m1.tests[-1]['test'])
print(m1.tests[-1]['pvalue'])
print(m1.tests[-1]['summary'])

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
m2.test(type="arch", quiet=True)
print(m2.tests[-1]['type'])
print(m2.tests[-1]['options'])
print(m2.tests[-1]['test'])
print(m2.tests[-1]['pvalue'])

print("\n****************************")
print("normality")
m2.test(type="normality", quiet=True)

print("\n****************************")
print("autocorr")
m2.test(type="autocorr", order=4)

print("\n****************************")
print("corrgm")
m2.test(type="corrgm")

# PANEL
print("\n****************************")
print("panspec")
m3 = gretl.ols(formula="invest ~ const value kstock", data=d3).fit()
print(m3)
m3.test(type="panspec")

print("\n****************************")
print("panel")
m3.test(type="panel")

print("\n****************************")
print(f'In m1 there is total of {len(m1.tests):d} tests')
print(f'In m2 there is total of {len(m2.tests):d} tests')
print(f'In m3 there is total of {len(m3.tests):d} tests\n')

# RESTRICT
print("\n****************************")
print("restrict")
m4 = gretl.ols(formula='1 0 2 3 4 5', data=d4).fit()
m4.restrict(impose=['b[4] - b[5] = 0', 'b[3] = 0'])
print(m4.restrictions[-1]['pvalue'])

print("\n****************************")
print("coeffsum")
m4.test(type="coeffsum", vars="WHITE ADMEXP")
