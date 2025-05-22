import importlib.resources as resources
import gretl

def run_example():
    data_dir = resources.files('gretl').joinpath('data')
    d1 = gretl.get_data(str(data_dir.joinpath('kennan.gdt')))
    d1.set_as_default()

    m1 = gretl.duration('Ti ~ const + Prod').fit()
    m2 = gretl.duration('Ti const Prod').fit()
    m3 = gretl.duration('1 0 2').fit()
    print(m1)
    print(m2)
    print(m3)

    m = gretl.duration('Ti ~ const + Prod', exponential=True).fit()
    print(m)

    m = gretl.duration('Ti ~ const + Prod', loglogistic=True).fit()
    print(m)

    m = gretl.duration('Ti ~ const + Prod', lognormal=True).fit()
    print(m)

    m = gretl.duration('Ti ~ const + Prod', medians=True).fit()
    print(m)

    m = gretl.duration('Ti ~ const + Prod', robust=True).fit()
    print(m)

    m = gretl.duration('Ti ~ const + Prod', vcv=True).fit()
    print(m)

    m = gretl.duration('Ti ~ const + Prod', verbose=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
