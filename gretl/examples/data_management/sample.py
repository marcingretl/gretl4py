import gretl

def run_example():
  # get path object for the data directory inside the gretl package
  d1 = gretl.get_data('longley.csv')
  d1.sample(5, 10)            # select a subsample from time 5 to time 10
  d1.print()                  # print the current subsample

  # we restore the full sample
  d1.sample(full=True)       # reset the sample to full dataset

  # let's use only obs for which armfrc >= 2000
  d1.sample('armfrc>=2000', restrict=True, replace=True)  # restrict sample to obs satisfying armfrc >= 2000, replacing previous sample

if __name__ == "__main__":
    run_example()
