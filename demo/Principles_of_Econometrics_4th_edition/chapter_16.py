try:
  from tabulate import tabulate
except ModuleNotFoundError:
  exit('*** \'tabulate\' package required ***\n')
import numpy as np
import gretl

# check gretl4py version
assert float(gretl.version) >= 0.41, "required gretl4py 0.41 or higher"

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
transport = gretl.get_data('data/transport.gdt')
coke = gretl.get_data('data/coke.gdt')
nels_small = gretl.get_data('data/nels_small.gdt')
olympics = gretl.get_data('data/olympics.gdt')
tobit = gretl.get_data('data/tobit.gdt')
mroz = gretl.get_data('data/mroz.gdt')

############################################
# 16.1 Models with Binary Dependent Variable
############################################
print("\n16.1. Models with Binary Dependent Variable")
print("\n16.1.5. A transportation Example")
PROB = gretl.probit(formula="auto const dtime").fit()
print(PROB)

############################################
# 16.2 The Logit Model for Binary Choice
############################################
print("\n16.2. The Logit Model for Binary Choice")
print("\n16.2.1. An Empirical Example from Marketing")
coke.set_as_default()
spec = "coke const pratio disp_coke disp_pepsi"
coke_logit = gretl.logit(formula=spec).fit()
coke_probit = gretl.probit(formula=spec).fit()
coke_lpm = gretl.ols(formula=spec).fit()
gretl.modeltab([coke_lpm, coke_probit, coke_logit])

print("\n16.2.2. Wald Hypothesis Tests")
coke_probit.restrict(impose=['b[3] + b[4] = 0'])
coke_probit.restrict(impose=['b[3] = 0', 'b[4] = 0'])

############################################
# 16.3 Multinomial Logit
############################################
print("\n16.3. Multinomial Logit")
nels_small.set_as_default()
NS_ML = gretl.logit(formula="psechoice const grades", multinomial=True).fit()
data = list(zip(['b12', 'b22', 'b13', 'b23'], NS_ML.coeff, NS_ML.stderr, NS_ML.coeff / NS_ML.stderr))
print("Table 16.2 Maximum Likelihood Estimates of PSE Choice")
print(tabulate(data, headers=["Parameters", "Estimates", "Stdandard errors", "t-Statitics"], tablefmt="plain", floatfmt=".4f"))
print("\n\nTable 16.3 *** TO BE ADDED!!!! ***\n\n")

############################################
# 16.5 Ordered Choice Models
############################################
print("\n16.5. Ordered Choice Models")
print("\n16.5.3. An Example")
NS_OP = gretl.probit(formula="psechoice const grades").fit()
print(NS_OP)

############################################
# 16.6 Models for Count Data
############################################
print("\n16.6. Models for Count Data")
print("\n16.5.3. An Example")
olympics.set_as_default()
olympics.sample("year==88", restrict=True)
OL_P = gretl.poisson(formula="medaltot const log(pop) log(gdp)").fit()
print(OL_P)
olympics.sample(full=True)

############################################
# 16.7 Limited Dependent Variables
############################################
print("\n16.7. Limited Dependent Variables")
tobit.set_as_default()
TOB = gretl.tobit(formula="y const x").fit()    # (16.34)
print(TOB)

print("\n16.7.5. An Example")
mroz.set_as_default()
spec = "hours const educ exper age kidsl6"
mroz_ols = gretl.ols(formula=spec).fit()
mroz.sample("hours>0", restrict=True)
mroz_ols_r = gretl.ols(formula=spec).fit()
mroz.sample(full=True)
mroz_tobit = gretl.tobit(formula=spec).fit()
print("Table 16.8 Estimates of Labor Supply Function")
gretl.modeltab([mroz_ols, mroz_ols_r, mroz_tobit])

print("\n16.7.6. Sample Selection")
mroz_wage_ols = gretl.ols(formula="log(wage) const educ exper", simple=True).fit()  # (16.43)
print(mroz_wage_ols)
mroz.new_series("kids = kidsl6 + kids618>0")
mroz_wage_he = gretl.heckit("log(wage) const educ exper ; lfp const age educ kids mtr", two_step=True).fit()  # (16.44)
print(mroz_wage_he)
mroz_wage_he_ml = gretl.heckit("log(wage) const educ exper ; lfp const age educ kids mtr").fit()
print(mroz_wage_he_ml)
