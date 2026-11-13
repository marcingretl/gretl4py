import gretl

gretl.set("verbose", "off")
gretl.set("seed", 231761287)

# We load/install+load GFE package
try:
    gfe = gretl.include("GFE")
except FileNotFoundError:
    print("Installing the 'GFE' package...")
    gretl.pkg("install", "GFE")
    gfe = gretl.include("GFE")

d = gretl.get_data("Acemoglu_etal.gdt", frompkg="GFE")

X = ['lag_dem', 'lag_income']
G = 15

### BIC-based model selection  --- this essentially replicates Table
### S.XI in the Supplement to Bonhomme-Manresa (2015)

print('GFE started...')

opts = gretl.Bundle(tvar=1, algorithm=2, verbose=1, randstarts=24, BICselect=1)

gretl.set("stopwatch")
mod = gretl.gfe_estimate('fhpolrigaug', X, G, opts)
gretl.gfe_printout(mod, True)
print(f'total execution time: {gretl.get_accessor("$stopwatch")}')
