import importlib.resources as resources
import gretl

# get path object for the data directory inside the gretl package
data_dir = resources.files('gretl').joinpath('data')

# we force libgretl to use the decimal point character
gretl.set('force_decpoint', 'on')

#d1 = gretl.get_data(gretl.gretl_data_path('misc/mrw.gdt'))
#d1.new_series("ly60 = log(gdp60)")
#d1.new_series("dlny = log(gdp85) - ly60")
#d1.new_series("ngd = 0.05 + (popgrow/100)")
#d1.new_series("lngd = log(ngd)")
#d1.new_series("linv = log(i_y/100)")
#d1.new_series("lschool = log(school/100)")
#print(d1.varnames())
#m1 = gretl.quantreg(tau=0.75, formula='dlny const ly60 linv lngd lschool').fit()
#print(m1)

d2 = gretl.get_data(str(data_dir.joinpath('bjg.gdt')))
d2.set_as_default()
m2 = gretl.quantreg(tau=0.45, formula='1 0 2').fit()
print(m2)
m3 = gretl.quantreg(tau=[0.45, 0.75, 0.99], formula='1 0 2').fit()
print(m3)
