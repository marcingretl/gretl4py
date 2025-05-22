import importlib.resources as resources
import gretl

def run_example():
    data_dir = resources.files('gretl').joinpath('data')
    d1 = gretl.get_data(str(data_dir.joinpath('greene25_1.gdt')))
    d1.set_as_default()

    m = gretl.biprobit(
        'anydrg cardhldr const age avgexp ; const age income ownrent selfempl').fit()
    print(m)

    m = gretl.biprobit(
        'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
        vcv=True).fit()
    print(m)

    m = gretl.biprobit(
        'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
        robust=True).fit()
    print(m)

    m = gretl.biprobit(
        'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
        opg=True).fit()
    print(m)

    m = gretl.biprobit(
        'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
        save_xbeta=True).fit()
    print(m)
    print(m.bp_yhat)

    m = gretl.biprobit(
        'anydrg cardhldr const age avgexp ; const age income ownrent selfempl',
        verbose=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
