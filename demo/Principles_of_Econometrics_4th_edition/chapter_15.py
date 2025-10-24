import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
nls_panel = gretl.get_data('data/nls_panel.gdt')

############################################
# 15.1. A Microeconomic Panel
############################################
print("\n15.1. A Microeconomic Panel")
print("Table 15.1 Representative Observations from NSL Panel Data")
nls_panel.sample(t1=1, t2=15)
nls_panel.print(series='lwage educ south black union exper tenure', byobs=True)
nls_panel.sample(full=True)

############################################
# 15.2. Pooled Model
############################################
print("\n15.2. Pooled Model")
print("\n15.2.2. Pooled Least Squares Estimates of Wage Equation")
spec = 'lwage const educ exper exper2 tenure tenure2 black south union'
POLS = gretl.panel(formula=spec, pooled=True).fit()
POLS_robust = gretl.panel(formula=spec, pooled=True, robust=True).fit()
print("\nTable 15.2 Pooled Least Squares Estimates of Wage Equation")
mdls = [POLS, POLS_robust]
gretl.modeltab(mdls)

