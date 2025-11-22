import math
import gretl

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# datasets
usa = gretl.get_data('data/usa.gdt')
spurious = gretl.get_data('data/spurious.gdt')

############################################
# 12.1. Stationary and Nonstationary Variables
############################################
print("\n12.1. Stationary and Nonstationary Variables")
print("Table 12.1")
usa.sample(t1='1984:2', t2='1996:4')
gretl.summary(simple=True)
usa.sample(t1='1997:1', t2='2009:4')
gretl.summary(simple=True)
usa.sample(full=True)

############################################
# 12.2. Spurious Regression
############################################
print("\n12.2. Spurious Regression")
spurious.set_as_default()
SP = gretl.ols('rw1 const rw2', simple=True).fit()
print(SP)

############################################
# 12.3. Unit Root Tests for Stationary
############################################
print("\n12.3. Unit Root Tests for Stationary")
print("\n12.3.6. The Dickey-Fuller Test: An Example")
usa.set_as_default()
usa.diff('f')
usa.test(type='adf', order=1, vars='f b', c=True, verbose=True)

print("\n12.3.7. Order of Integration")
usa.test(type='adf', vars='f b', nc=True, difference=True, verbose=True)

############################################
# 12.4. Cointegration
############################################
print("\n12.4. Cointegration")
usa.test(type='coint', order=1, y='b', vars='f', verbose=True)      # (12.9)

print("\n12.4.2. The Error Correction Model")
usa.diff('b')
usa.lags(vars="f b")
spec: gretl.NLSpec = {
    "formula": "d_b = -a * (b_1 - b1 - b2 * f_1) + d0 * d_f + d1 * d_f(-1)",
    "params": 'a b1 b2 d0 d1',
    "initvals": {
        "a" : 0,
        "b1" : 0,
        "b2": 0,
        "d0": 0,
        "d1": 0,
    },
}
usa_nls = gretl.nls(spec).fit()         # (12.10)
print(usa_nls)
