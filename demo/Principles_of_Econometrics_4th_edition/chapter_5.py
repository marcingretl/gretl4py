import math
import numpy
import gretl

# check gretl4py version
assert float(gretl.version) >= 0.31, "required gretl4py 0.31 or higher"

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# NumPy precision
numpy.set_printoptions(precision = 4)

# datasets
andy = gretl.get_data('data/andy.gdt')
pizza = gretl.get_data('data/pizza4.gdt')
cps4s = gretl.get_data('data/cps4_small.gdt')

############################################
# 5.1. Introduction
############################################
print("\n5.1. Introduction")
print("Table 5.1 Observations on Monthly Sales, Price, and Advertising in Big Andy's Burger Barn")
andy.head()
andy.tail(3)
gretl.summary(simple=True)

############################################
# 5.2. Estimating the Parameters of the Multiple Regression Model
############################################
print("\n5.2. Estimating the Parameters of the Multiple Regression Model")
print("\n5.2.2. Least Squares Estimates Using Hamburger Chain Data")
m1 = gretl.ols(formula='sales ~ const + price + advert').fit()                  # (5.6)
print(m1)
# alternative ways of defining a model
m2 = gretl.ols('sales ~ 0 + price + advert').fit()
m3 = gretl.ols('sales const price advert').fit()
m4 = gretl.ols('sales 0 price advert').fit()
m5 = gretl.ols('1 const 2 3').fit()
m6 = gretl.ols('1 0 2 3').fit()
m7 = gretl.ols('1 ~ const + 2 + 3').fit()
m8 = gretl.ols('1 ~ 0 + 2 + 3').fit()

# using gretl list
andy.new_list(name='Xes1', value='price advert')
andy.new_list(name='Xes2', value='2 3')

m9 = gretl.ols('sales const Xes1').fit()
m10 = gretl.ols('1 0 Xes2').fit()

# coefficients estimates
for i in range(0,3):
    print(f'b{i} = {m1.coeff[i]:.5} ({m1.stderr[i]:.4})')

# SSE
print(f'SSE = {m1.ess:.7}')

# standard errors
stderr = m1.stderr
print(f'sigmahat = {m1.sigma:.5}')

############################################
# 5.3. Sampling Properties of the Least Squares Estimator
##########################################
print("\n5.3. Sampling Properties of the Least Squares Estimator")
print("\n5.3.1. The Variances and Covariances of the Least Squares Estimator")

# covariance matrix
print(f'\ncovariance matrix:\n{m1.vcv}')                                        # (5.10)
for i in range(0,3):
    print(f'var(b{i}) = {m1.vcv[i][i]:.5}')

############################################
# 5.4. Interval Estimation
############################################
print("\n5.4. Interval Estimation")
print("\n5.4.1. Interval Estimation for the Single Coefficient")
critval = gretl.critical('t', value=0.025, params=m1.df)
print(f'({m1.coeff[1] - critval * m1.stderr[1]:.5}, \
{m1.coeff[1] + critval * m1.stderr[1]:.5})')                                    # (5.16)

############################################
# 5.5. Hypothesis Testing
############################################
print("\n5.5. Hypothesis Testing")
print("\n5.5.3. Hypothesis Testing for Linear Combination of Coefficients")
m1.restrict(impose=['-0.2*b[2] - 0.5*b[3] = 0'])
print(f'-0.2*b[2] - 0.5*b[3] > 0, pvalue: {m1.restrictions[-1]['pvalue']/2 : .3f}')
b = m1.coeff
vcv = m1.vcv
t = (-0.2 * b[1] - 0.5 * b[2]) / math.sqrt((-0.2)**2 * vcv[1][1] + (-0.5)**2 * vcv[2][2] + 2 * -0.2 * -0.5 * vcv[1][2])
pval = gretl.pvalue(d='t', value=t, params=72)
print(f't = {t:.3f}, pvalue = {pval:.3f}')

############################################
# 5.6. Polynomial Equations
############################################
print("\n5.6. Polynomial Equations")
print("\n5.6.2. Extending the Model for Hamburger Barn Sales")
andy.square('advert')
msq = gretl.ols(formula='sales ~ const + price + advert + sq_advert').fit()      # (5.24)
print(msq)

############################################
# 5.7. Interaction Variables
############################################
print("\n5.7. Interaction Variables")
pizza.set_as_default()
print("Table 5.4 Pizza Expenditure Data")
pizza.print(series="pizza income age", range="1:5", byobs=True)
pizza.new_series('ageincome = age * income')
PZ_1 = gretl.ols('pizza const age income', simple=True).fit()                   # (5.27)
PZ_2 = gretl.ols('pizza const age income ageincome', simple=True).fit()         # (5.28)
print(PZ_1)
print(PZ_2)

print("\n5.7.1. Log-Linear Model")
cps4s.set_as_default()
cps4s.square('educ exper', cross=True)
CPS = gretl.ols('ln(wage) const educ exper educ_exper', simple=0).fit()          # (5.30)
print(CPS)
