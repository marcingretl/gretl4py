import gretl

gretl.set('verbose', 'off')

d = gretl.get_data("greene11_3.gdt")

# NLS
DERIV=['a = 1', 'b = Y^g', 'g = b * Y^g * log(Y)']
PARAMS = 'a b g'
INIT_VALS = dict()              # we pass initial values as dictionary
INIT_VALS["a"] = 1
INIT_VALS["b"] = -1
INIT_VALS["g"] = 1

m1 = gretl.nls("C = a + b * Y^g", deriv=DERIV, initvals=INIT_VALS).fit()
m2 = gretl.nls(function="C = a + b * Y^g", params=PARAMS, initvals=INIT_VALS, vcv=True).fit()
print(m1)
print(m2)

print(m1.get_formula())
print(m2.get_formula())

# MLE
gretl.genr("p = 0.5")           # we pass initial value as libgretl's uv
d.new_series("X = uniform() >= 0.5 ? 1 : 0")
DERIV = ['p = X/p - (1-X)/(1-p)']
m3 = gretl.mle("ll = X*log(p) + (1-X)*log(1-p)", deriv=DERIV).fit()
m4 = gretl.mle(loglik="ll = X*log(p) + (1-X)*log(1-p)", params='p', robust=True).fit()
print(m3)
print(m4)

print(m3.get_formula())
print(m4.get_formula())
