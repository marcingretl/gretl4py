import numpy

import sys
sys.path.append("../")
import gretl

print("Python level:")
arr = numpy.array([[1, 2, 3], [4, 5, 6]]) #, numpy.double)
#arr = numpy.array([1, 2, 3]) #, numpy.double) ''' Must throw an exception since it is 1-dim vector!!! '''
print(arr)

print("\nC++ level:")
gretl.load_matrix(arr)