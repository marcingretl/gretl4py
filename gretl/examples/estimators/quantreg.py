import gretl


def run_example():
    # Force libgretl to use the decimal point character.
    gretl.set("force_decpoint", "on")

    # Load dataset.
    d1 = gretl.get_data("mrw.gdt")

    # Construct variables.
    d1.new_series("ly60 = log(gdp60)")
    d1.new_series("dlny = log(gdp85) - ly60")
    d1.new_series("ngd = 0.05 + (popgrow/100)")
    d1.new_series("lngd = log(ngd)")
    d1.new_series("linv = log(i_y/100)")
    d1.new_series("lschool = log(school/100)")

    # Restrict sample to non-oil countries.
    d1.sample("nonoil", dummy=True)

    # Model specification.
    spec = "dlny const ly60 linv lngd lschool"

    # Quantile regression (top quartile).
    m_top = gretl.quantreg(tau=0.75, formula=spec).fit()

    # Quantile regression (bottom quartile, robust SE).
    m_bot = gretl.quantreg(tau=0.25, formula=spec, robust=True).fit()

    print(m_top)
    print(m_bot)

    # Quantile regression at multiple quantiles.
    m_multi = gretl.quantreg(tau=[0.45, 0.75, 0.99], formula=spec).fit()
    print(m_multi)


if __name__ == "__main__":
    run_example()
