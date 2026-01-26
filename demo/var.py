import gretl

def run_example():
    # get path object for the data directory inside the gretl package
    d1 = gretl.get_data('denmark.gdt')
    d1.sample(1, 51)

    #print(d1.varnames())
    m1 = gretl.var(order=2, ylist='LRM LRY', xlist='IBO IDE', trend=True, seasonals=True, impulse_responses=True).fit()
    print(m1)

    # we retrieve sigma matrix
    print("sigma matrix:")
    print(m1.get_accessor("$sigma"))

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
    uhat_name = m1.save_uhat(eqn=1)
    print(f'residuals added as: {uhat_name}')

    #FORECASTS
    print("\n****************************")
    print("forecasts")
    m1.fcast(vname="LRY")

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

        y = m1.irfs[-1]['results'][:, 0]
        l = m1.irfs[-1]['results'][:, 1]
        u = m1.irfs[-1]['results'][:, 2]
        x = list(range(1, len(y) + 1))
        shock_name = m1.irfs[-1]['shock_name']
        target_name = m1.irfs[-1]['target_name']

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
