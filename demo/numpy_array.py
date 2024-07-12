import numpy

import sys
sys.path.append("../")
import gretl

print("Python level:")
arr = numpy.array([[1, 2, 3], [4, 5, 6]]) #, numpy.double)
#arr = numpy.array([1, 2, 3]) #, numpy.double)
print(arr)

print("\nC++ level:")
print("fortranarray (column-major):")
gretl.load_data(numpy.asfortranarray(arr))
print("row-major:")
gretl.load_data(arr)
