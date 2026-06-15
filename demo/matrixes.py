import gretl

# Pandas DF
import pandas as pd

A = gretl.Matrix(4, 4, 5)
print(type(A))
print(A)
print(A.rnameget())
print(A.cnameget())

A_np = A.numpy_view
print(type(A_np))
print(A_np)
A_np[1,2] = -1000
print(A_np)
print(A)

B = gretl.Matrix(1.0, 5.0)
print(B)

C = gretl.Matrix(10, 2, "Uniform")
print(C)
C_np = C.to_numpy()
C_np.resize(2, 10)
print(C_np)

D = gretl.Matrix(2, 3)
D.rnameset(['first', 'second'])
D.cnameset(['one', 'two', 'three'])
pd = D.to_pandas()
pdv = D.pandas_view
D.numpy_view[0,0] = 55
D[1,1] = 22
print(D)
print(D.cnameget()[0])
D.cnameset(list())
print(D)
print("as Pandas DF")
print(pd)
print(pdv)

# complex
Z = gretl.Matrix(2, 3, 1.0, "complex")
print(Z)

