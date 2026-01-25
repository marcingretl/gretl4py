import gretl

def run_example():
    # Force libgretl to use the decimal point character
    gretl.set('force_decpoint', 'on')

    d1 = gretl.get_data('mrw.gdt')
    d1.new_series('ly60 = log(gdp60)')
    d1.new_series('dlny = log(gdp85) - ly60')
    d1.new_series('ngd = 0.05 + (popgrow/100)')
    d1.new_series('lngd = log(ngd)')
    d1.new_series('linv = log(i_y/100)')
    d1.new_series('lschool = log(school/100)')
    d1.sample('nonoil', dummy=True)
    
    spec = 'dlny const ly60 linv lngd lschool'
    
    # Top quartile
    m1_top = gretl.quantreg(tau=0.75, formula=spec).fit()
    # Bottom quartile (robust)
    m1_bot = gretl.quantreg(tau=0.25, formula=spec, robust=True).fit()
    print(m1_top)
    print(m1_bot)
    
    # Fit quantile regression at multiple taus
    m2 = gretl.quantreg(tau=[0.45, 0.75, 0.99], formula=spec).fit()
    print(m2)

if __name__ == "__main__":
    run_example()
