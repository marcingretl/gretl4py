import gretl

d1 = gretl.get_data(gretl.gretl_data_path('misc/denmark.gdt'))
d2 = gretl.get_data(gretl.gretl_data_path('misc/bjg.gdt'))
#gretl.show_data_status()
#print(d1.get_accessor("$pd"))

# OLS
m1 = gretl.ols(formula='1 0 2 3 4').fit()
print(m1)
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

