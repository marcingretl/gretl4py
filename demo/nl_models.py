import gretl

gretl.set('verbose', 'off')

d1 = gretl.get_data("greene11_3.gdt")
d2 = gretl.get_data("hall.gdt")
d3 = gretl.get_data("kennan.gdt")

# NLS
DERIV=['a = 1', 'b = Y^g', 'g = b * Y^g * log(Y)']
PARAMS = 'a b g'
PARAM_NAMES = 'par1 par2 par3'
INIT_VALS = dict()              # we pass initial values as dictionary
INIT_VALS["a"] = 1
INIT_VALS["b"] = -1
INIT_VALS["g"] = 1

m1 = gretl.nls("C = a + b * Y^g", deriv=DERIV, initvals=INIT_VALS).fit()
m2 = gretl.nls(function="C = a + b * Y^g", params=PARAMS, initvals=INIT_VALS, param_names=PARAM_NAMES, vcv=True).fit()
print(m1)
print(m2)

print(m1.get_formula())
print(m2.get_formula())

# MLE
gretl.genr("p = 0.5")           # we pass initial value as libgretl's uv
d1.new_series("X = uniform() >= 0.5 ? 1 : 0")
DERIV = ['p = X/p - (1-X)/(1-p)']
PARAM_NAMES = 'probability'

spec3: gretl.NLSpec = {
    "formula": "ll = X*log(p) + (1-X)*log(1-p)",
    #"params": 'p',
    "param_names": "probability",    
    "deriv": {
        "p": "X/p - (1-X)/(1-p)"
    },
    "initvals": {
        "p" : 0.5,
    },
}
m3 = gretl.mle(spec3, robust=True).fit()

d3.set_as_default()
d3.new_series("l_Ti = log(Ti)")
spec4: gretl.NLSpec = {
    "formula": "loglik = Z - log(s) - exp(Z)",
    "definitions": [
        'series Xb = b0 + b1*Prod',
        'series Z = (l_Ti - Xb) / s',
        'series W = -(1 - exp(Z)) / s'
    ],
    "deriv": {
        "b0": "W",
        "b1": "Prod * W",
        "s":  "-Z/s - 1/s + exp(Z) * Z/s"
    },
    "initvals": {
        "b0" : 2,
        "b1" : -5,
        "s":  1
    },
}
m4 = gretl.mle(spec4, hessian=True).fit()

print(m3)
print(m4)
print(m3.get_formula())
print(m4.get_formula())

# GMM
d2.set_as_default()
d2.new_series("e = 0")
d2.new_list(name="inst", value="const consrat(-1) consrat(-2) ewr(-1) ewr(-2)")
gretl.genr(formula="matrix V0 = 100000*I(nelem(inst))")
INIT_VALS["alpha"] = 0.5
INIT_VALS["delta"] = 0.5
ORTHOG = ['e ; inst']
WEIGHTS = 'V0'
PARAMS = 'alpha delta'

m5 = gretl.gmm("e = delta*ewr*consrat^(alpha-1) - 1", orthog=ORTHOG, params=PARAMS, weights=WEIGHTS, initvals=INIT_VALS).fit()

print(m5)
print(m5.get_formula())
