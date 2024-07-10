import numpy

import sys
sys.path.append("../")
import gretl

print("Python level:")
arr = numpy.array([[1, 2, 3], [4, 5, 6]]) #, numpy.double)
#arr = numpy.array([1, 2, 3]) #, numpy.double)
print(arr)

print("\nC++ level:")
print("non fortranarray:")
gretl.load_matrix(numpy.asfortranarray(arr))
print("non asfortranarray:")
gretl.load_matrix(arr)
