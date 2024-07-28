import sys
sys.path.append("../")
import time
import numpy
import gretl

total_fstyle = 0.0
total_cstyle = 0.0

Nrepl = 1000

Ns = [50, 100, 500, 1000, 5000, 10000]
ks = [5, 10, 15, 20, 25, 30]

for i in range(Nrepl):
    for N in Ns:
        for k in ks:
            arr = numpy.array(numpy.random.rand(N,k))
            t0 = time.time()
            gretl.numpy_array_test(numpy.asfortranarray(arr))
            t1 = time.time() - t0
            total_fstyle += t1
            t0 = time.time()
            gretl.numpy_array_test(arr)
            t1 = time.time() - t0
            total_cstyle += t1

print(f'total time for f_style: {total_fstyle:f}')
print(f'total time for c_style: {total_cstyle:f}')
print(f'ratio: {total_fstyle/total_cstyle:f}')

