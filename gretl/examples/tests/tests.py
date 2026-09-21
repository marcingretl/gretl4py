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
  res_difftest = d5.difftest(x="capp", y="cdis", rank_sum=True)

  print("\n****************************")
  print("fractint")
  res_fractint = d1.fractint(series="LRM")

  print("\n****************************")
  print("johansen")
  res_johansen = d1.johansen(order=4, varlist="LRM LRY IBO IDE", rc=True)

  print("\n****************************")
  print("kpss")
  res_kpss = d1.kpss(order=1, varlist="LRM", seasonals=True, trend=True)

  print("\n****************************")
  print("levinlin")
  res_levinlin = d3.levinlin(series="invest", order=[2,2,3,3,4,4,2,3,2,1])

  print("\n****************************")
  print("meantest")
  res_meantest = d3.meantest(x="invest", y="firm")
  print(res_meantest['pvalue'])

  print("\n****************************")
  print("normtest")
  res_normtest = d1.normtest(series="LRY", swilk=True)

  print("\n****************************")
  print("runs")
  res_runs = d1.runs(series="LRY", difference=True)

  print("\n****************************")
  print("vartest")
  res_vartest = d1.vartest(x="LRM", y="IBO")

  ###############################################################
  # model based tests
  ###############################################################

  # OLS
  m1 = gretl.ols(formula='1 0 2 3').fit()
  print(m1)

  print("\n****************************")
  res_add = m1.add(varlist="IDE", silent=True)
  print(res_add['type'])
  print(res_add['options'])
  print(res_add['test'])
  print(res_add['pvalue'])
  print(res_add['summary'])

  print("\n****************************")
  print("bkw")
  res_bkw = m1.bkw()

  print("\n****************************")
  print("breusch-pagan")
  res_breusch_pagan = m1.breusch_pagan(robust=True)

  print("\n****************************")
  print("chow")
  res_chow = m1.chow(target="1980:1")

  print("\n****************************")
  print("cusum")
  res_cusum = m1.cusum(quiet=True)
  print(res_cusum['pvalue'])

  print("\n****************************")
  print("leverage")
  res_leverage = m1.leverage()

  print("\n****************************")
  print("nonlinearity")
  res_nonlinearity = m1.nonlinearity(mode="squares")

  print("\n****************************")
  print("normality")
  res_normality = m1.normality()
  print(res_normality)

  print("\n****************************")
  print("omit")
  res_omit = m1.omit(varlist="3")

  print("\n****************************")
  print("qlrtest")
  res_qlrtest = m1.qlrtest()

  print("\n****************************")
  print("reset")
  res_reset = m1.reset(cubes_only=True, robust=True)

  print("\n****************************")
  print("white")
  res_white = m1.white(white_nocross=True)

  print("\n****************************")
  print("vif")
  res_vif = m1.vif()

  # ARIMA
  m2 = gretl.arima(y='lg', order=(2,0,0), data=d2).fit()
  print(m2)

  print("\n****************************")
  res_arch = m2.arch(quiet=True)
  print(res_arch)

  print("\n****************************")
  print("autocorr")
  res_autocorr = m2.autocorr(order=4)

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
