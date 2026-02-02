import gretl

def run_example():
  d1 = gretl.get_data("greene19_1.gdt")

  # to take obs between 1 and 13
  d2 = d1[1:13] # d['1:13'] works as well
  print(d2)
  print(d2.n)
  print("----------")

  # to slice via some logical expression
  d3 = d1['TUCE>20']
  print(d3)
  print(d3.n)
  print("----------")

  d4 = d1['PSI==1']
  print(d4)
  print(d4.n)
  print("----------")

  # to take only specified variables
  d5 = d1[['GPA', 'PSI']]
  print(d5)
  print(d5.varnames())
  print("----------")

  # slicers intersection
  d5 = d1[['GPA', 'PSI']][12:23]
  print(d5)
  print(d5.varnames())
  d5.head(6)

  # data status
  gretl.show_data_status()

if __name__ == "__main__":
    run_example()
