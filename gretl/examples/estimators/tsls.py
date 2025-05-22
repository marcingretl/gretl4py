import importlib.resources as resources
import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    # Load the dataset explicitly and set it as default
    d1 = gretl.get_data(str(data_dir.joinpath('penngrow.gdt')))
    d1.set_as_default()

    m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)').fit()
    print(m)

    # Don’t do diagnostic tests
    m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', no_tests=True).fit()
    print(m)

    # Print covariance matrix
    m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', vcv=True).fit()
    print(m)

    # No degrees-of-freedom correction
    m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', no_df_corr=True).fit()
    print(m)

    # Robust standard errors
    m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', robust=True).fit()
    print(m)

    # Uncomment to use Limited Information Maximum Likelihood
    # m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', liml=True).fit()
    # print(m)

    # Uncomment to use Generalized Method of Moments
    # m = gretl.tsls('Y Y(-1) X ; 0 X Y(-2)', gmm=True).fit()
    # print(m)

if __name__ == "__main__":
    run_example()
