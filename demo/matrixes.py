import gretl

A = gretl.Matrix(4, 4, 5)
print(type(A))
A.print()
print(A.rnameget())
print(A.cnameget())

A_np = A.numpy_view
print(type(A_np))
print(A_np)
A_np[1,2] = -1000
print(A_np)
A.print()

B = gretl.Matrix(1.0, 5.0)
B.print()

C = gretl.Matrix(10, 2, "Uniform")
C.print()

D = gretl.Matrix(0, 0, 1.6)
D.print()
