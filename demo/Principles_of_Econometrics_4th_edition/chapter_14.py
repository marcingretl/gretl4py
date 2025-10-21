import gretl

if float(gretl.version) < 0.31:
  exit("required gretl4py 0.31 or higher")

# switch of verbosity
gretl.set('verbose', 'off')

# force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

# we load gig addon
GIG = gretl.include("gig")

# datasets
byd = gretl.get_data('data/byd.gdt')

############################################
# 14.3. Testing, Estimating, and Forecasting
############################################
print("\n14.3.2. Estimating ARCH Models")
byd.print(summary=True)
ARCH_1 = gretl.garch('0 1 ; r').fit()                               # (14.4a-14.4b)
print(ARCH_1)

############################################
# 14.4. Extensions
############################################
print("\n14.4.1. The GARCH Model -- Generalized ARCH")
GARCH_1_1 = gretl.garch('1 1 ; r').fit()                            # (14.4a-14.4b)
print(GARCH_1_1)

print("\n14.4.2. Allowing For an Asymmetric Effect")
byd.new_list(name='X', value='const')
byd.new_list(name='Y', value='null')
GARCH_1_1_asym_setup = gretl.gig_setup(y='r', type=3, X='X', Y='Y', ARlags=0)
GARCH_1_1_asym = gretl.gig_estimate(model=GARCH_1_1_asym_setup, verbose=1)
