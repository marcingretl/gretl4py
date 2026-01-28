import gretl
import numpy as np

def run_example():
  # define  bundle (call class constructor)
  a = gretl.Bundle()
  b = gretl.Bundle(xxx="example string")
  print(a)            # flat print
  b.print()           # equivalent to print(b)

  # add items to bundle
  l = [[1.5, 4.78], [5, -0.6]]
  b['mat'] = np.asarray(l)      # assign an array-like object; can be a list, tuple, or numpy array
  a['hhh'] = 464645
  a['ddd'] = "fhfg"
  a['inner_bundle'] = b
  print(b)
  a.print(tree=True)            # expanded tree view

  # delete element
  el = b.pop('xxx')               # pop returns removed value
  print(f"Pop-ed element: {el}")
  del a['ddd']                    # delete via __delitem__
  print(b)
  a.print(tree=True)

  # export to dict
  d = a.to_dict()
  print("Exported dict:\n", d)

if __name__ == "__main__":
    run_example()
