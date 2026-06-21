import gretl
import numpy as pd
import time

rows = 500
cols = 500
rep = 10 ** 3

A = gretl.Matrix(rows, cols, "normal")
B = gretl.Matrix(rows, cols, "normal")

A_np = A.to_numpy().reshape(rows, cols, order='C')
B_np = B.to_numpy().reshape(rows, cols, order='C')

t0 = time.time()
for i in range(0,rep):
    A @ B
t1 = time.time() - t0

t0 = time.time()
for i in range(0,rep):
    A_np @ B_np
t2 = time.time() - t0

print(f'GretlMatrix:\t{t1}\nNumPy:\t\t{t2}\nratio:\t\t{t1/t2:.4}')
