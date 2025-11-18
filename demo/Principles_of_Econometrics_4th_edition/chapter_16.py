import gretl

# switch of verbosity
try:
  from tabulate import tabulate
except ModuleNotFoundError:
  exit('*** \'tabulate\' package required ***\n')
import numpy as np
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
transport = gretl.get_data('data/transport.gdt')
coke = gretl.get_data('data/coke.gdt')
nels_small = gretl.get_data('data/nels_small.gdt')
olympics = gretl.get_data('data/olympics.gdt')
tobit = gretl.get_data('data/tobit.gdt')

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
print("Table 16.2 Maximum Likellyhood Estimates of PSE Choice")
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
olympics.sample(restriction="year==88")
OL_P = gretl.poisson(formula="medaltot const log(pop) log(gdp)").fit()
print(OL_P)
olympics.sample(full=True)

############################################
# 16.7 Limited Dependent Variables
############################################
print("\n16.7. Dependent Variables")
tobit.set_as_default()
TOB = gretl.tobit(formula="y const x").fit()    # (16.34)
print(TOB)

