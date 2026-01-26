import gretl

def run_example():
    # load a dataset
    d1 = gretl.get_data('denmark.gdt')
    d1.sample(1, 51)

    m1 = gretl.vecm(order=2, rank=1, ylist='LRM LRY', xlist='IBO IDE', rc=True, seasonals=True).fit()
    print(m1)

    # we retrieve sigma matrix
    print("jalpha:")
    print(m1.get_accessor("$jalpha"))

    # we perform some model-based tests
    m1.test(type="autocorr", order=3)
    print("test statistics:")
    print(m1.tests[-1]['test'])

    m1.test(type="arch")
    print("pvalues:")
    print(m1.tests[-1]['pvalue'])

    m1.test(type="normality", silent=True)
    print(m1.tests[-1]['summary'])

    # add uhat to dataset
    uhat_name = m1.save_uhat(eqn=2)
    print(f'residuals added as: {uhat_name}')

    # add EC term to dataset
    ec_name = m1.save_ec_term(nr=1)
    print(f'EC term added as: {ec_name}')
    
    # RESTRICT
    print("\n****************************")
    print("restrict")
    m1.restrict(impose=['b[2] = 0'], verbose=True)
    print(m1.restrictions[-1]['pvalue'])

    #FORECASTS
    print("\n****************************")
    print("forecasts")
    m1.fcast()

    #IRFS
    print("\n****************************")
    print("IRFs")
    m1.irf(target=0, shock=0)
    m1.irf(target='LRM', shock='LRY', alpha=0.1)
    m1.irf(target=1, shock=0, alpha=0.1)

    print("\nirf1:")
    print(m1.irfs[0])

    print("\nirf2:")
    print(m1.irfs[1])

    print("\nirf3:")
    print(m1.irfs[-1])
    
    # IRFs plots (requires matplotlib)
    try:
        import matplotlib.pyplot as plt

        irf_res = m1.irfs[-1]
        y = irf_res['results'][:, 0]
        l = irf_res['results'][:, 1]
        u = irf_res['results'][:, 2]
        x = list(range(1, len(y) + 1))
        shock_name = irf_res['shock_name']
        target_name = irf_res['target_name']

        fig, ax = plt.subplots()
        plt.title(target_name + ": responses to a one-standard error shock in " + shock_name)
        ax.plot(x, y, '-')
        ax.fill_between(x, y - l, y + u, alpha=0.2)
        ax.plot(x, y, 'o', color='tab:brown')
        plt.show()
    except ModuleNotFoundError:
        print("please install 'matplotlib' module")

if __name__ == "__main__":
    run_example()
