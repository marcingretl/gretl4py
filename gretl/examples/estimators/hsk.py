import gretl

def run_example():
    # load a dataset
    d = gretl.get_data('data4-1')
    
    m = gretl.hsk('price const sqft bedrms').fit()
    print(m)

if __name__ == "__main__":
    run_example()
