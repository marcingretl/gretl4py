import sys
sys.path.append("../")
import time
import numpy
import gretl

total_fstyle = 0.0
total_cstyle = 0.0
f_style_trans = 0.0

Nrepl = 1000

Ns = [50, 100, 500, 1000, 5000, 10000]
ks = [5, 10, 15, 20, 25, 30]

for i in range(Nrepl):
    for N in Ns:
        for k in ks:
            arr = numpy.array(numpy.random.rand(N,k))
            t0 = time.time()
            arr2 = numpy.asfortranarray(arr)
            t2 = time.time() - t0
            f_style_trans += t2
            t0 = time.time()
            gretl.numpy_array_test(arr2)
            t1 = time.time() - t0 + t2
            total_fstyle += t1
            t0 = time.time()
            gretl.numpy_array_test(arr)
            t1 = time.time() - t0
            total_cstyle += t1

print(f'total time for f_style: {total_fstyle:f} (including asfortranarray() time: {f_style_trans:f})')
print(f'total time for c_style: {total_cstyle:f}')
print(f'ratio: {total_fstyle/total_cstyle:f}')

