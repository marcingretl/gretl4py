import gretl

A = gretl.Matrix(4, 4, 1.0)

print(type(A))
A.print()

print(type(A.numpy_view))
print(A.numpy_view)

B = gretl.Matrix(1.0, 5.0)
B.print()

C = gretl.Matrix(10, 2, "normal")
C.print()
