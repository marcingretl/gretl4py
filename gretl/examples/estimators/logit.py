import importlib.resources as resources
import gretl

def run_example():
    data_dir = resources.files('gretl').joinpath('data')

    d1 = gretl.get_data(str(data_dir.joinpath('greene22_2.gdt')))
    d1.set_as_default()

    m1 = gretl.logit('Y ~ const + Z1 + Z2').fit()
    m2 = gretl.logit('Y const Z1 Z2').fit()
    m3 = gretl.logit('1 0 2 3').fit()
    print(m1)
    print(m2)
    print(m3)

    # robust standard errors
    m = gretl.logit('Y ~ const + Z1 + Z2', robust=True).fit()
    print(m)

    # estimate multinomial logit
    m = gretl.logit('Y ~ const + Z1 + Z2', multinomial=True).fit()
    print(m)

    # print covariance matrix
    m = gretl.logit('Y ~ const + Z1 + Z2', vcv=True).fit()
    print(m)

    # print details of iterations
    m = gretl.logit('Y ~ const + Z1 + Z2', verbose=True).fit()
    print(m)

    # show p-values instead of slopes
    m = gretl.logit('Y ~ const + Z1 + Z2', p_values=True).fit()
    print(m)

    # select pseudo-R-squared variant
    m = gretl.logit('Y ~ const + Z1 + Z2', estrella=True).fit()
    print(m)

    # oddsratios analysis
    d2 = gretl.get_data(str(data_dir.joinpath('rac3d.gdt')))
    m = gretl.logit('SEX 0 AGE AGESQ INCOME LEVYPLUS', data=d2).fit()
    print(m)
    m.oddsratio()

if __name__ == "__main__":
    run_example()
