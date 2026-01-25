import gretl

def run_example():
    # Get path object for the data directory inside the gretl package
    d1 = gretl.get_data('tobit.gdt')

    # Estimate Tobit model with both left and right censoring
    m1 = gretl.tobit('apt ~ const + read + math + proggeneral + progvocational',
                     llimit=400, rlimit=600.7).fit()
    print(m1)

if __name__ == "__main__":
    run_example()
