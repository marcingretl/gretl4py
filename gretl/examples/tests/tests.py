import gretl

def run_example():
  # datasets used
  d1 = gretl.get_data('denmark.gdt')
  d2 = gretl.get_data('bjg.csv')
  d3 = gretl.get_data('grunfeld.gdt')
  d4 = gretl.get_data('data4-10.gdt')
  d5 = gretl.get_data('ooballot.gdt')
  gretl.show_data_status()

  ###############################################################
  # dataset based tests
  ###############################################################
  print("\n****************************")
  print("adf")
  res_adf = d1.adf(varlist="LRM", order=1, quiet=True)
  print(res_adf['summary'])

  print("\n****************************")
  print("bds")
  res_bds = d1.bds(order=4, x="LRY", quiet=False)
  print(res_bds['summary'])

  print("\n****************************")
  print("coint")
  res_coint = d1.coint(depvar="LRM", indepvars="LRY IBO", order=4)

  print("\n****************************")
  print("corrgm")
  res_corrgm = d1.corrgm(y="LRM", order=8)
  print(res_corrgm['pvalue'])

  print("\n****************************")
  print("difftest")
  d5.new_series("capp = (Vote > 1) ? CPI : NA")
  d5.new_series("cdis = (Vote == 0) ? CPI : NA")
  d5.difftest(x="capp", y="cdis", rank_sum=True)

  print("\n****************************")
  print("fractint")
  d1.fractint(series="LRM")

  print("\n****************************")
  print("johansen")
  d1.johansen(order=4, varlist="LRM LRY IBO IDE", rc=True)

  print("\n****************************")
  print("kpss")
  d1.test(type="kpss", varlist="LRM", order=1, seasonals=True, trend=True)

  print("\n****************************")
  print("levinlin")
  d3.test(type="levinlin", varlist="invest", order=[2,2,3,3,4,4,2,3,2,1])

  print("\n****************************")
  print("meantest")
  d3.test(type="meantest", varlist="invest firm")
  print(d3.tests[-1]['pvalue'])

  print("\n****************************")
  print("normtest")
  d1.test(type="normtest", varlist="LRY", swilk=True)

  print("\n****************************")
  print("runs")
  d1.test(type="runs", varlist="LRY", difference=True)

  print("\n****************************")
  print("vartest")
  d1.test(type="vartest", varlist="LRM IBO")

  ###############################################################
  # model based tests
  ###############################################################

  # OLS
  m1 = gretl.ols(formula='1 0 2 3').fit()
  print(m1)

  print("\n****************************")
  m1.test(type="add", varlist="IDE", silent=True)
  print(m1.tests[-1]['type'])
  print(m1.tests[-1]['options'])
  print(m1.tests[-1]['test'])
  print(m1.tests[-1]['pvalue'])
  print(m1.tests[-1]['summary'])

  print("\n****************************")
  print("bkw")
  m1.test(type="bkw")

  print("\n****************************")
  print("chow")
  m1.test(type="chow", obs="1980:1")

  print("\n****************************")
  print("cusum")
  m1.test(type="cusum", quiet=True)

  print("\n****************************")
  print("leverage")
  m1.test(type="leverage")

  print("\n****************************")
  print("omit")
  m1.test(type="omit", varlist="3")

  print("\n****************************")
  print("qlrtest")
  m1.test(type="qlrtest")

  print("\n****************************")
  print("reset")
  m1.test(type="reset", cubes_only=True, robust=True)

  print("\n****************************")
  print("white")
  m1.test(type="white")

  print("\n****************************")
  print("vif")
  m1.test(type="vif")

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
  print("autocorr")
  m2.test(type="autocorr", order=4)

  print("\n****************************")
  print("corrgm")
  m2.test(type="corrgm")

  ######################
  # PANEL specific
  ######################
  m3 = gretl.ols(formula="invest ~ const value kstock", data=d3).fit()
  print(m3)
  print("\n****************************")
  print("panel")
  m3.test(type="panel")

  print("\n****************************")
  print("panspec")
  m3.test(type="panspec")

  ######################
  # RESTRICT
  ######################
  m4 = gretl.ols(formula='1 0 2 3 4 5', data=d4).fit()
  print(m4)
  print("\n****************************")
  print("coeffsum")
  m4.test(type="coeffsum", varlist="WHITE ADMEXP")

  print("\n****************************")
  print("restrict")
  m4.restrict(impose=['b[4] - b[5] = 0', 'b[3] = 0'])
  print(m4.restrictions[-1]['pvalue'])

  m5 = gretl.vecm(order=2, rank=1, ylist='LRM LRY', xlist='IBO IDE', rc=True, seasonals=True, data=d1).fit()
  m5.restrict(impose=['b[2] = 0'], verbose=True)
  print(m5.restrictions[-1]['pvalue'])

  ###############################################################
  # summary
  ###############################################################
  print("\n*************************************************")
  print(f'In m1 there is total of {len(m1.tests):d} tests')
  print(f'In m2 there is total of {len(m2.tests):d} tests')
  print(f'In m3 there is total of {len(m3.tests):d} tests')
  print(f'In m4 there is total of {len(m4.tests):d} tests')
  print(f'In m4 there is total of {len(m4.restrictions):d} restrictions')
  print(f'In m5 there is total of {len(m5.restrictions):d} restrictions')
  print("*************************************************")

if __name__ == "__main__":
    run_example()
