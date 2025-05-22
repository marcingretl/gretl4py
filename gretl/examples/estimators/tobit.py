import importlib.resources as resources
import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    data_dir = resources.files('gretl').joinpath('data')

    # Load the dataset explicitly and set it as default
    d1 = gretl.get_data(str(data_dir.joinpath('tobit.gdt')))
    d1.set_as_default()

    # Estimate Tobit model with both left and right censoring
    m1 = gretl.tobit('apt ~ const + read + math + proggeneral + progvocational',
                     llimit=400, rlimit=600.7).fit()
    print(m1)

if __name__ == "__main__":
    run_example()
